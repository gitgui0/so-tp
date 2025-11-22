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


void listaUsers(User* users, int nUsers){
    printf("\n--- LISTA DE UTILIZADORES ---\n");
    for(int i = 0; i < nUsers; i++){
        printf("%d. %s (PID: %d)\n", i+1, users[i].nome, users[i].pid_cliente);
    }
    printf("-----------------------------\n");
}


// Return 0 - sucesso
// Return 1 - erro genérico
// Return 2 - já existe
// Return 3 - lista cheia
int existeEAdicionaUser(User* users, int* nUsers, char* nome){
    if(*nUsers < 0) return 1;
    if(*nUsers >= MAX_USERS) return 3;
    
    for(int i = 0; i < *nUsers; i++){
        if(strcmp(users[i].nome, nome) == 0)
            return 2; // Já existe
    }

    // Adiciona novo utilizador
    strcpy(users[*nUsers].nome, nome);
    (*nUsers)++; // Incrementa o contador global
    return 0;
}

void handleSinal(int sinal, siginfo_t *info, void *context){
    (void)info; (void)context; // Silenciar warnings de variaveis nao usadas
    if(sinal == SIGINT){
        printf("\n[SINAL] SIGINT recebido. A terminar...\n");
        loop = 0;
    }
    if(sinal == SIGALRM){
        tempo++;
        alarm(1); // Re-agendar o alarme
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

    if (strcmp(comando, "listar") == 0) {
        // TODO: Implementar listagem de serviços
        printf("Comando listar (servicos) - A implementar\n");
    }
    else if (strcmp(comando, "utiliz") == 0) {
        // CORREÇÃO THREADS: Acede diretamente à memória protegida
        pthread_mutex_lock(&users_mutex);
        listaUsers(users, nUsers);
        pthread_mutex_unlock(&users_mutex);
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
}




// --- Thread: Processamento de Clientes ---
void* tUsers(void* arg) {
    // 1. CAST E EXTRAÇÃO DOS ARGUMENTOS
    TUserInfo *info = (TUserInfo*)arg;
    int fd_leitura_fifo = info->fd_pipe;
    volatile int *loop_ptr = info->loop_ptr;
    pthread_mutex_t *mutex = info->users_mutex_ptr;
    
    free(arg); // Liberta a memória da struct alocada na main

    // Variáveis locais
    char buffer_msg[256];
    char conf[5];
    char cmd[20], nome[MAX_STR], fifo_nome[MAX_PIPE];
    pid_t pid_cli;
    
    printf("[THREAD - USER] Clientes iniciada. À espera de dados no FIFO...\n");

    while (*loop_ptr) { 
        int nbytes = read(fd_leitura_fifo, buffer_msg, sizeof(buffer_msg) - 1);
        
        if (nbytes > 0) {
            buffer_msg[nbytes] = '\0'; 
            
            int res_scan = sscanf(buffer_msg, "%s %s %s %d", cmd, nome, fifo_nome, &pid_cli);

            if (res_scan == 4 && strcmp(cmd, "LOGIN") == 0) {
                
                // PROTEÇÃO (LOCK)
                pthread_mutex_lock(mutex);
                
                if (existeEAdicionaUser(users, &nUsers, nome) == 0) {
                    //Copia as informacoes para o array de clientes 
                    strcpy(users[nUsers - 1].fifo_privado, fifo_nome);
                    users[nUsers - 1].pid_cliente = pid_cli;
                    strcpy(conf, LOGIN_SUCESSO);
                } else {
                    strcpy(conf, "ERRO"); 
                }
                
                // DESBLOQUEAR
                pthread_mutex_unlock(mutex); 

                //Envia para o cliente a confirmacao

                int fd_cliente = open(fifo_nome, O_WRONLY); 
                if (fd_cliente != -1) {
                    write(fd_cliente, conf, strlen(conf));
                    close(fd_cliente);
                } else {
                    printf("[ERRO] Nao conseguiu abrir FIFO de resposta %s.\n", fifo_nome);
                }
                
                printf("[LOGIN] %s -> Resultado: %s\n", nome, conf);
                
            } else {
                // Aqui pode tratar AGENDAR, CANCELAR, etc.
                if (nbytes > 1) printf("[AVISO] Comando desconhecido: %s\n", buffer_msg);
            }
        } 
        else if (nbytes == -1 && errno != EINTR) {
            perror("[ERRO] Leitura do FIFO principal");
            sleep(1); // Evita spam em caso de erro
        }
    }
    
    printf("[THREAD] Clientes a terminar.\n");
    close(fd_leitura_fifo);
    return NULL;
}

void* tControl(void* arg) {
    TControlInfo* info = (TControlInfo*)arg;
    
    free(arg); // Liberta a memória da struct alocada na main

    // Variáveis locais
    char buffer[MAX_STR];
    
    printf("[THREAD - CONTROL] Thread control iniciada. À espera de dados no FIFO...\n");

    while(info->loop_ptr){
        printf("\nAdmin> ");
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
    int fd_pipe_controlador; 
    
    setbuf(stdout,NULL);

    char* s_nveiculos = getenv("NVEICULOS");
    if(s_nveiculos == NULL) {
        max_veiculos = MAX_VEICULOS; 
    } else {
        max_veiculos = atoi(s_nveiculos); 
        if (max_veiculos <= 0) max_veiculos = MAX_VEICULOS;
        
        if (max_veiculos > MAX_VEICULOS) {
            printf("AVISO: NVEICULOS (%d) excede o limite compilado. A usar %d.\n", max_veiculos, MAX_VEICULOS);
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
    fd_pipe_controlador = open(PIPE_CONTROLADOR, O_RDWR); 
    if(fd_pipe_controlador == -1){
        perror("Erro fatal ao abrir FIFO");
        unlink(PIPE_CONTROLADOR);
        exit(EXIT_FAILURE);
    }else{
        printf("PIPE CONTROLADOR FOI ABERTO\n");
    }

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