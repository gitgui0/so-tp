#include "comum.h"

// --- Variáveis Globais ---
int loop = 1;
volatile int tempo = 0;

// Dados partilhados entre threads
User users[MAX_USERS]; 
int nUsers = 0;

// Array para Serviços e Frota
Servico servicos[MAX_SERVICOS];
int nServicos = 0;
int max_veiculos;

Veiculo frota[MAX_VEICULOS];
int nVeiculos = 0;

//Threads
pthread_t control_tid;
pthread_t clientes_tid;
pthread_t tempo_tid;
pthread_t frota_tid;

// Sincronização
pthread_mutex_t users_mutex;
pthread_mutex_t servicos_mutex;
pthread_mutex_t frota_mutex;


User* devolveUserPorNome(char* nome){
    for(int i = 0; i < nUsers; i++){
        if(strcmp(users[i].nome, nome) == 0){
            return &users[i];
        }
    }
    return NULL;
}

User* devolveUserPorPID(pid_t pid){
    for(int i = 0; i < nUsers; i++){
        if(users[i].pid_cliente == pid){
            return &users[i];
        }
    }
    return NULL;
}

Servico* devolveServicoPorID(int id){
    for(int i = 0; i < nServicos; i++){
        if(servicos[i].id == id){
            return &servicos[i];
        }
    }
    return NULL;
}

Servico* devolveServicoPorUserID(pid_t pidUser){
    for(int i = 0; i < nServicos; i++){
        if(servicos[i].pid_cliente == pidUser){
            return &servicos[i];
        }
    }
    return NULL;
}

Veiculo* devolveVeiculoPorPID(pid_t pid){
    for(int i = 0; i < nVeiculos; i++){
        if(frota[i].pid_veiculo == pid){
            return &frota[i];
        }
    }
    return NULL;
}


void listaUsers(){
    printf("\n--- LISTA DE UTILIZADORES ---\n");
    for(int i = 0; i < nUsers; i++){
        Servico *s = devolveServicoPorUserID(users[i].pid_cliente);
        char estado[MAX_STR];

        if(s==NULL) strcpy(estado, "Sem servico");
        else {
            if(s->estado == SERV_AGENDADO) strcpy(estado, "A espera de veiculo");
            else if(s->estado == SERV_EM_CURSO) strcpy(estado, "Em viagem");
        }

        printf("%d. %s (PID: %d) - %s\n", i+1, users[i].nome, users[i].pid_cliente,estado);
    }
    printf("-----------------------------\n");
}

void listaServicos(){
    printf("\n--- LISTA DE SERVICOS ---\n");
    for(int i = 0; i < nServicos; i++){
        printf("(ID: %d) %d - %dkm -  %s - Estado: ",servicos[i].id, servicos[i].hora_agendada, servicos[i].distancia, servicos[i].origem);
        if(servicos[i].estado == SERV_AGENDADO)
            printf("Agendado ");
        else if(servicos[i].estado == SERV_EM_CURSO)
            printf("Em curso ");
        else if(servicos[i].estado == SERV_CONCLUIDO)
            printf("Concluido ");
        printf("- Para %s (%d)\n",devolveUserPorPID(servicos[i].pid_cliente)->nome, servicos[i].pid_cliente);
    }
    printf("-----------------------------\n");
}

void listaFrotaAtiva(){
    printf("\n--- LISTA DE VEICULOS ATIVOS ---\n");
    for(int i = 0; i < nVeiculos; i++){
        if(frota[i].estado == VEICULO_OCUPADO){
            printf("Veiculo(%d) - %.2f - %d km de %d km\n",
            frota[i].pid_veiculo, (float)frota[i].distancia_percorrida / frota[i].distancia * 100,
            frota[i].distancia_percorrida, frota[i].distancia);
        }
    }
    printf("-----------------------------\n");
}



// Return 0 - sucesso
// Return 1 - erro genérico
// Return 2 - já existe
// Return 3 - lista cheia
int existeEAdicionaUser(char* nome, char* fifo, pid_t pid){
    if(nUsers >= MAX_USERS) return 3;
    
    for(int i = 0; i < nUsers; i++){
        if(strcmp(users[i].nome, nome) == 0)
            return 2; // Já existe
    }

    // Adiciona novo utilizador
    strcpy(users[nUsers].nome, nome);
    strcpy(users[nUsers].fifo_privado, fifo); 
    users[nUsers].pid_cliente = pid;
    users[nUsers].ativo = 1;
    nUsers++; // Incrementa o contador global
    return 0;
}


// -1 - nenhum veiculo livre
int devolveVeiculoLivre(){
    for(int i = 0; i < nVeiculos; i++){
        if(frota[i].estado == VEICULO_LIVRE){
            return i; 
        }
    }
    return -1; 
}

// Envia resposta sem bloquear
void enviaResposta(char* nome_user, char* msg){
    char fifo[MAX_PIPE];
    fifo[0] = '\0';

    for(int i=0; i<nUsers; i++){
        if(users[i].ativo && strcmp(users[i].nome, nome_user) == 0){
            strcpy(fifo, users[i].fifo_privado);
            break;
        }
    }

    if(strlen(fifo) > 0){
        int fd = open(fifo, O_WRONLY); 
        if(fd != -1){
            write(fd, msg, strlen(msg));
            close(fd);
        }
    }
}


// --- TRATAMENTO DE COMANDOS ---
void tratarComandoCliente(char* cmd, char* nome, char* args) {
    
    if (strcmp(cmd, "LOGIN") == 0) {
        char fifo[MAX_PIPE];
        int pid;
        sscanf(args, "%s %d", fifo, &pid);
        
        int res = existeEAdicionaUser(nome, fifo, pid);
        char resposta[MAX_STR];
        
        if(res == 0 || res == 2) strcpy(resposta, LOGIN_SUCESSO);
        else strcpy(resposta, "ERRO: Cheio");

        int fd_c = open(fifo, O_WRONLY);
        if(fd_c != -1){
            write(fd_c, resposta, strlen(resposta));
            close(fd_c);
        }
        printf("\n[LOGIN] User: %s", nome);

    }
    else if(strcmp(cmd,"agendar") == 0){
        Servico novo;
        memset(&novo, 0, sizeof(Servico));

        
        //destino?
        sscanf(args, "%d %s %d", &novo.hora_agendada,  novo.origem, &novo.distancia);

        novo.id = nServicos + 1;
        novo.estado = SERV_AGENDADO;
        novo.pid_cliente = devolveUserPorNome(nome)->pid_cliente;
        novo.pid_veiculo = -1;
        servicos[nServicos] = novo;
        nServicos++;

    }
    else {
        printf("\n[AVISO] Comando desconhecido: %s", cmd);

    }
    printf("\nAdmin> ");
    fflush(stdout);
}

void handleSinal(int sinal, siginfo_t *info, void *context){
    (void)info; (void)context; // Silenciar warnings de variaveis nao usadas
    if(sinal == SIGINT){
        printf("\n[SINAL] SIGINT recebido.\n");
        pthread_cancel(control_tid);
        loop = 0;
    }
    if(sinal == SIGALRM){
        tempo++;
        alarm(1); 
    }
}


// --- Processamento Comandos Admin (Executado pela thread Main) ---
void processar_comando_admin(char* buffer) {
    char comando[MAX_STR];
    int id_cancelar = -1; 

    buffer[strcspn(buffer, "\n")] = 0; // Remove \n
    
    // Limpa variaveis
    memset(comando, 0, MAX_STR);
    
    int args = sscanf(buffer, "%s %d", comando, &id_cancelar);
    if (args <= 0) return;

    // CORREÇÃO THREADS: Acede diretamente à memória protegida
    pthread_mutex_lock(&users_mutex);

    if (strcmp(comando, "listar") == 0) {
        pthread_mutex_lock(&servicos_mutex);
        listaServicos();
        pthread_mutex_unlock(&servicos_mutex);
    }
    else if (strcmp(comando, "utiliz") == 0) {
        listaUsers();
    }
    else if (strcmp(comando, "frota") == 0) {
        listaFrotaAtiva();
    }
    else if (strcmp(comando, "km") == 0) {
        int count = 0;
        for(int i = 0; i < nVeiculos; i++){
            if(frota[i].estado == VEICULO_OCUPADO){
                count += frota[i].distancia_percorrida;
            }
        }
        printf("Distancia total percorrida por todos os veiculos: %d km\n", count);
        
    }
    else if (strcmp(comando, "hora") == 0) {
        printf("Tempo simulado: %d\n", tempo);
    }
    else if (strcmp(comando, "cancelar") == 0 && args == 2) {
        printf("A cancelar servico ID %d (A implementar logicamente)\n", id_cancelar);
    }
    else if (strcmp(comando, "terminar") == 0) {
        loop = 0;
    }
    else {
        printf("Comando desconhecido: %s\n", comando);
    }
    fflush(stdout);
    pthread_mutex_unlock(&users_mutex);
}


// --- Thread: Processamento de Clientes ---
void* tUsers(void* arg) {
    // 1. CAST E EXTRAÇÃO DOS ARGUMENTOS
    TUserInfo *info = (TUserInfo*)arg;
    int fd_leitura_fifo = info->fd_pipe;
    volatile int *loop_ptr = info->loop_ptr;
    pthread_mutex_t *mutex = info->users_mutex_ptr;
    
    free(arg); // Liberta a memória da struct alocada na main

    printf("\n[THREAD - CLIENTE] Thread cliente iniciada.\n");

    // Variáveis locais
    char buffer_msg[256];
    char cmd[20], nome[MAX_STR], args[MAX_STR * 2];

    while (*loop_ptr) { 
        int nbytes = read(fd_leitura_fifo, buffer_msg, sizeof(buffer_msg) - 1);
        
        if (nbytes > 0) {
            buffer_msg[nbytes] = '\0'; 
            
            
            int res_scan = sscanf(buffer_msg, "%s %s %[^\n]", cmd, nome, args);
            printf("\n[THREAD-USER] MENSAGEM RECEBIDA: %s\n", buffer_msg);

            if(res_scan < 2) {
                printf("\nMsg invalida: %s\nAdmin> ", buffer_msg);
                continue;
            }

            // PROTEÇÃO (LOCK)
            pthread_mutex_lock(mutex);
            tratarComandoCliente(cmd, nome, args);
            // LIBERTAR (UNLOCK)
            pthread_mutex_unlock(mutex);    
            } 
        else if (nbytes == -1 && errno != EINTR) {
            perror("[ERRO] Leitura do FIFO principal");
            sleep(1);
        }
    }
    close(fd_leitura_fifo);
    return NULL;
}

void* tControl(void* arg) {
    TControlInfo* info = (TControlInfo*)arg;

    // Variáveis locais
    char buffer[MAX_STR];
    
    printf("\n[THREAD - CONTROL] Thread control iniciada.\n");
    fflush(stdout);

    while(info->loop_ptr){
        printf("Admin> ");
        if(fgets(buffer, sizeof(buffer), stdin) == NULL){
            if (errno == EINTR) continue;
            loop = 0; // EOF (Ctrl+D)
            break;     
        }
        processar_comando_admin(buffer);

        // no caso de ter sido o comando termina
        if(loop==0){
            break;
        }
    }
    
    printf("[THREAD] Thread control a terminar.\n");
    return NULL;
}

void* tTempo(void* arg){

    while(loop){
        // so para testar
        // printf("\n%d", tempo);
        // Verificar para todos os servicos, se o tempo chegou a hora do servico
       
        
        for(int i = 0; i < nServicos; i++){
            // <= ou == ?
            // meti <= so para estar seguro
            
            pthread_mutex_lock(&servicos_mutex);
            if(servicos[i].estado == SERV_CONCLUIDO){
                memset(&servicos[i], 0 , sizeof(Servico));
                nServicos--;
            }else if(servicos[i].estado == SERV_AGENDADO && servicos[i].hora_agendada <= tempo){  
                // Iniciar servico
                if(nVeiculos >= max_veiculos){
                    printf("Nao ha veiculos livres e ja atingimos o maximo de veiculos (%d)\n", max_veiculos);
                    continue;
                } else {
                    Veiculo novo;
                    memset(&novo, 0, sizeof(Veiculo));
                    novo.id_servico = servicos[i].id;
                    novo.estado = VEICULO_OCUPADO;
                    int fd_v[2];
                    if(pipe(fd_v) == -1){
                        perror("Erro ao criar pipe veiculo-controlador");
                        continue;
                    }
                    pid_t pid = fork();
                    if(pid == -1){
                        perror("Erro ao criar processo veiculo");
                        continue;
                    }
                    // Processo Filho - Veiculo
                    else if(pid== 0){
                        
                        // Argumentos
                        // ./veiculo <id> <origem> <distancia> <fifo_cliente>
                        close(fd_v[0]); // fecha leitura
                        char str_id[MAX_STR];
                        char str_distancia[MAX_STR];
                        char str_fd[MAX_STR];
                        char pipe[MAX_PIPE];
                        sprintf(str_id, "%d", servicos[i].id);
                        sprintf(str_distancia, "%d", servicos[i].distancia);
                        sprintf(str_fd, "%d", fd_v[1]);
                        User* u = devolveUserPorPID(servicos[i].pid_cliente);
                        sprintf(pipe, "%s", u->fifo_privado);
                        
                        execl("./veiculo", "veiculo", str_id, servicos[i].origem, str_distancia, str_fd, pipe, NULL);
                        
                        perror("[ERRO CRITICO] execl falhou");
                        exit(EXIT_FAILURE);
                    }
                    // Processo Pai - Controlador
                    else{
                        close(fd_v[1]); // fecha escrita
                        novo.id_servico = servicos[i].id;
                        novo.estado = VEICULO_OCUPADO;
                        novo.pid_veiculo = pid;
                        novo.fd_leitura = fd_v[0];
                        novo.distancia = servicos[i].distancia;
                        novo.distancia_percorrida = 0;
                        
                        int flags = fcntl(fd_v[0], F_GETFL, 0);
                        fcntl(fd_v[0], F_SETFL, flags | O_NONBLOCK);
                        
                        
                        frota[nVeiculos] = novo;
                        nVeiculos++;
                        servicos[i].pid_veiculo = pid;
                        servicos[i].estado = SERV_EM_CURSO;
                        
                        User* u = devolveUserPorPID(servicos[i].pid_cliente);
                        printf(" Veiculo (%d) iniciado para %s(%d) \n", novo.pid_veiculo, u->nome, u->pid_cliente);
                        
                    }
                    
                }
           }
           pthread_mutex_unlock(&servicos_mutex);
        }
        
        sleep(1); // NAO TENHO A CERTEZA DISTO MAS ACHO QUE FAZ SENTIDO
    }
    return NULL;
}

void *tTFrota(void* arg){
    while(loop){
        for(int i = 0; i < nVeiculos; i++){
            if(frota[i].estado == VEICULO_OCUPADO){
                char buf[MAX_STR];
                char msg[MAX_STR];
                int distanciaAtual, idServico = frota[i].id_servico;

                Servico* s = devolveServicoPorID(idServico);
                User* u = devolveUserPorPID(s->pid_cliente);

                /// caso para mensagem de progresso
                int nbytes = read(frota[i].fd_leitura, buf, sizeof(buf));
                
                if(nbytes > 0){
                    buf[nbytes] = '\0';

                    sscanf(buf, "%s %d", msg, &distanciaAtual);

                    if(strcmp(msg,VIAGEM_CONCLUIDA)==0){
                        frota[i].estado = VEICULO_LIVRE;
 
                        if(s != NULL){
                            s->estado = SERV_CONCLUIDO;

                            char resposta[MAX_STR];
                            printf("[THREAD-FROTA] Veiculo (%d) concluiu a viagem do %s(%d)\n", frota[i].pid_veiculo, u->nome, u->pid_cliente);                
                            
                            waitpid(frota[i].pid_veiculo, NULL, 0); // evitar zombies

                            memset(&frota[i], 0 , sizeof(Veiculo));
                            nVeiculos--;

                        }
                    }else if(strcmp(msg,"PROGRESSO")==0){
                        frota[i].distancia_percorrida = distanciaAtual;  
                    } else{
                        printf("\n[THREAD-FROTA] [VEICULO %d para %s(%d)]: %s\n", frota[i].pid_veiculo, u->nome, u->pid_cliente, msg);
                    }
                }
            }
        }
        sleep(1); // nao e ideal mas nao sei outra solucao
    }
    return NULL;
}

// --- MAIN ---
int main(){
    setbuf(stdout,NULL);

    char* s_nveiculos = getenv("NVEICULOS");
    if(s_nveiculos == NULL) 
        max_veiculos = MAX_VEICULOS; 
    else {
        max_veiculos = atoi(s_nveiculos); 
        if (max_veiculos <= 0) max_veiculos = MAX_VEICULOS;
        if (max_veiculos > MAX_VEICULOS) {
            printf("AVISO: NVEICULOS (%d) excede o limite. A usar %d.\n", max_veiculos, MAX_VEICULOS);
            max_veiculos = MAX_VEICULOS;
        }
    }
    printf("Max Veiculos = %d\n", max_veiculos);

    // 2. Sinais
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handleSinal;
    sa.sa_flags = SA_SIGINFO | SA_RESTART; // SA_RESTART ajuda a evitar falhas no read()
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);

    // 3. Preparar FIFO
    unlink(PIPE_CONTROLADOR); 

    if(mkfifo(PIPE_CONTROLADOR, 0666) == -1){ 
        if (errno != EEXIST) {
            perror("Erro fatal ao criar FIFO");
            exit(EXIT_FAILURE);
        }
    }

    // Abrir FIFO (O_RDONLY bloqueia até um cliente abrir para escrita, 
    // ou usamos O_RDWR para manter aberto sem clientes - truque comum)
    int fd_pipe_controlador = open(PIPE_CONTROLADOR, O_RDWR); 
    if(fd_pipe_controlador == -1){
        perror("Erro fatal ao abrir FIFO");
        unlink(PIPE_CONTROLADOR);
        exit(EXIT_FAILURE);
    };

    // 4. Iniciar Tempo e Mutex
    alarm(1);
    pthread_mutex_init(&users_mutex, NULL);
    pthread_mutex_init(&servicos_mutex, NULL);
    pthread_mutex_init(&frota_mutex, NULL);

    // 5. Lançar Thread Clientes
    TUserInfo *args_user = (TUserInfo*)malloc(sizeof(TUserInfo)); 
    if (args_user == NULL) {
        perror("Malloc falhou"); 
        close(fd_pipe_controlador);
        unlink(PIPE_CONTROLADOR);
        exit(EXIT_FAILURE);
    }
    args_user->fd_pipe = fd_pipe_controlador;
    args_user->loop_ptr = &loop;
    args_user->users_mutex_ptr = &users_mutex;

    if (pthread_create(&clientes_tid, NULL, tUsers, (void*)args_user) != 0) {
        perror("Erro ao criar thread para clientes.");
        exit(1);
    }

    TControlInfo* args_control = (TControlInfo*)malloc(sizeof(TControlInfo));
    if (args_control == NULL) {
        perror("Malloc falhou."); 
        close(fd_pipe_controlador);
        unlink(PIPE_CONTROLADOR);
        exit(EXIT_FAILURE);
    }

    args_control->loop_ptr=&loop; // nao e para tar assim acho eu
    args_control->servicos_mutex_ptr = &servicos_mutex;

    
    if (pthread_create(&control_tid, NULL, tControl, (void*)args_control) != 0) {
        perror("Erro ao criar thread para controlo do controlador");
        exit(1);
    }

    if (pthread_create(&tempo_tid, NULL, tTempo, NULL) != 0) {
        perror("Erro ao criar thread para controlo do controlador");
        exit(1);
    }

    if (pthread_create(&frota_tid, NULL, tTFrota, NULL) != 0) {
        perror("Erro ao criar thread para controlo do controlador");
        exit(1);
    }


    

    // pthread_cancel so para o ctrl c
    pthread_join(control_tid, NULL);

    // Cancelar a thread de clientes (que está bloqueada a ler o named pipe)
    pthread_cancel(clientes_tid); 
    pthread_join(clientes_tid, NULL);

    pthread_join(tempo_tid,NULL);
    pthread_join(frota_tid,NULL);

    // Fechar a sessao dos clientes que ainda estavam ativos
    for(int i = 0; i < nUsers; i++){
        if(users[i].ativo){
            kill(users[i].pid_cliente, SIGINT);
        }
    }
    
    
    // 3. Limpeza Final
    close(fd_pipe_controlador);
    unlink(PIPE_CONTROLADOR);
    pthread_mutex_destroy(&users_mutex);
    
    return 0;
} 