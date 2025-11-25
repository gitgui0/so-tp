#include "comum.h"

// --- Variáveis Globais ---
int loop = 1;
volatile int tempo = 0;

// Dados partilhados entre threads
User users[MAX_USERS]; 
int nUsers = 0;
int max_veiculos = 0;

// Array para Serviços e Frota
Servico servicos[MAX_SERVICOS];
int nServicos = 0;

Veiculo frota[MAX_VEICULOS];

// Sincronização
pthread_mutex_t users_mutex;


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


void listaUsers(){
    printf("\n--- LISTA DE UTILIZADORES ---\n");
    for(int i = 0; i < nUsers; i++){
        printf("%d. %s (PID: %d)\n", i+1, users[i].nome, users[i].pid_cliente);
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
        novo.pid_veiculo = -1; // por agora
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
        printf("\n[SINAL] SIGINT recebido. A terminar...\n");
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
        listaServicos();
    }
    else if (strcmp(comando, "utiliz") == 0) {
        listaUsers();
    }
    else if (strcmp(comando, "frota") == 0) {
        printf("Comando frota - A implementar\n");
    }
    else if (strcmp(comando, "km") == 0) {
        printf("Comando km - A implementar\n");
    }
    else if (strcmp(comando, "hora") == 0) {
        printf("Tempo simulado: %d\n", tempo);
    }
    else if (strcmp(comando, "cancelar") == 0 && args == 2) {
        printf("A cancelar servico ID %d (A implementar logicamente)\n", id_cancelar);
    }
    else if (strcmp(comando, "terminar") == 0) {
        printf("A terminar sistema...\n");
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
    char conf[5];
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
    
    free(arg); // Liberta a memória da struct alocada na main

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
    }
    
    printf("[THREAD] Thread control a terminar.\n");
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
    
    pthread_t clientes_tid;
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

    pthread_t control_tid;
    if (pthread_create(&control_tid, NULL, tControl, (void*)args_control) != 0) {
        perror("Erro ao criar thread para controlo do controlador");
        exit(1);
    }

    while(loop){
        sleep(1);
    }

    // 7. Encerramento
    printf("\nA encerrar o sistema...\n");

    // Fechar a sessao dos clientes que ainda estavam ativos
    for(int i = 0; i < nUsers; i++){
        if(users[i].ativo){
            kill(users[i].pid_cliente, SIGINT);
        }
    }
    
    // 1. Cancelar a thread de Clientes (que está bloqueada no read do FIFO)
    pthread_cancel(clientes_tid); 
    pthread_join(clientes_tid, NULL);

    // 2. Cancelar a thread de Controlo (que está bloqueada no fgets do Stdin)
    // O pthread_cancel é a única forma de interromper o fgets bloqueante
    pthread_cancel(control_tid);
    pthread_join(control_tid, NULL);
    
    // 3. Limpeza Final
    close(fd_pipe_controlador);
    unlink(PIPE_CONTROLADOR);
    pthread_mutex_destroy(&users_mutex);
    
    return 0;
} 