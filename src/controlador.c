#include "comum.h"

int loop=1;
volatile int tempo=0;
User users[MAX_USERS]; 
int nUsers = 0,max_veiculos = 0;
pid_t pid_filho;

void listaUsers(User* users, int nUsers){
    printf("LISTA USERS\n");
    for(int i = 0; i < nUsers; i++){
        printf("%d. %s\n",i+1,users[i].nome);
    }
    printf("FIM LISTA USERS\n");
}

void handleSinal(int sinal, siginfo_t *info, void *context){
    if(sinal == SIGINT){
        printf("SIGINT\n");
        loop = 0;
    }
    if(sinal == SIGALRM){
        tempo++;
        alarm(1);
    }
    if (sinal == SIGUSR1){
        listaUsers(users, nUsers);
    }

}

//Return 0 - tudo certo
//Return 1- erro
//Return 2 - ja existe
// Return 3 - cheio
int existeEAdicionaUser(User* users, int* nUsers, char* nome){
    if(*nUsers < 0) return 1;
    if(*nUsers >= MAX_USERS) return 3;
    for(int i = 0; i < *nUsers; i++){
        if(strcmp(users[i].nome, nome) == 0)
            return 2;
    }

    strcpy(users[*nUsers].nome,nome);

    (*nUsers)++;
    return 0;
}

void processar_comando_admin(char* buffer) {
    char comando[MAX_STR];
    int id_cancelar = -1; 

    // Tira o '\n' do fim
    buffer[strcspn(buffer, "\n")] = 0;
    memset(comando, 0, MAX_STR);
    int args = sscanf(buffer, "%s %d", comando, &id_cancelar);
    if (args <= 0) return;

    if (strcmp(comando, "listar") == 0) {
        printf("lista\n");
    }
    else if (strcmp(comando, "utiliz") == 0) {
        printf("A pedir lista de utilizadores ao processo filho...\n");
        union sigval val;
        val.sival_int = 0; // Não interessa o valor
        sigqueue(pid_filho, SIGUSR1, val); // Manda o sinal ao Filho
    }
    else if (strcmp(comando, "frota") == 0) {
        printf("frota\n");
    }
    else if (strcmp(comando, "km") == 0) {
        printf("kilemntreos\n");
    }
    else if (strcmp(comando, "hora") == 0) {
        printf("Tempo simulado = %d segundos\n", tempo);
    }
    else if (strcmp(comando, "cancelar") == 0 && args == 2) {
        printf("cancelar ID %d\n", id_cancelar);
    }
    else if (strcmp(comando, "terminar") == 0) {
        printf("termina\n");
        loop = 0;
    }
    else {
        printf("Comando desconhcido: %s\n", comando);
    }
}

int main(){
    User usr_tmp;
    int nbytes;
    char pipe[150], conf[5];
    
    setbuf(stdout,NULL);

    char* s_nveiculos = getenv("NVEICULOS");
    if(s_nveiculos == NULL) {
        printf("[AVISO] Variável NVEICULOS não definida. A assumir %d.\n", MAX_VEICULOS);
        max_veiculos = MAX_VEICULOS; 
    } else {
        max_veiculos = atoi(s_nveiculos); 
        if (max_veiculos <= 0 || max_veiculos > MAX_VEICULOS) {
            printf("[AVISO] NVEICULOS=%d inválido. A assumir %d.\n", max_veiculos, MAX_VEICULOS);
            max_veiculos = MAX_VEICULOS;
        }else{
            printf("NVEICULOS definido para %d veículos.\n", max_veiculos);
        }
    }

    struct sigaction sa_int;
    memset(&sa_int, 0, sizeof(struct sigaction));

    sa_int.sa_sigaction = handleSinal;
    sa_int.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa_int, NULL);


    struct sigaction sa_alrm = {0};
    sigemptyset(&sa_alrm.sa_mask);
    sa_alrm.sa_sigaction = handleSinal;     
    sa_alrm.sa_flags = SA_RESTART | SA_SIGINFO; // tem de ser com o restart
    sigaction(SIGALRM, &sa_alrm, NULL);


    if(access(PIPE_CONTROLADOR,F_OK) == 0){
        perror("Ja existe um processo controlador ativo.");
        return 0;
    }

    if(mkfifo(PIPE_CONTROLADOR, 0666) == -1){  // 0666 por agora
        perror("Erro ao criar pipe controlador.");
        exit(EXIT_FAILURE);
    }

    int fd_ler = open(PIPE_CONTROLADOR, O_RDWR);
    if(fd_ler == -1){
        perror("Erro a abrir pipe controlador.");
        unlink(PIPE_CONTROLADOR);
        exit(EXIT_FAILURE);
    }

    alarm(1);

    pid_filho = fork();
    if(pid_filho == -1){
        perror("Erro no fork.");
        close(fd_ler);
        unlink(PIPE_CONTROLADOR);
        exit(EXIT_FAILURE);
    }
   
    if (pid_filho == 0){

        struct sigaction sa_filho;
        memset(&sa_filho,0,sizeof(sa_filho));
        sa_filho.sa_sigaction = handleSinal;
        sa_filho.sa_flags = SA_RESTART | SA_SIGINFO;
        sigaction(SIGUSR1, &sa_filho,NULL);

        while(loop){
            memset(&usr_tmp, 0, sizeof(struct User));
            nbytes = read(fd_ler, usr_tmp.nome, sizeof(struct User));
            if(nbytes == -1){
                if (errno == EINTR){
                    continue;
                }
                printf("Erro ao ler utilizador.");
                continue;
            }

            int res = existeEAdicionaUser(users, &nUsers, usr_tmp.nome);
            
            if(res == 0){
                strcpy(conf, LOGIN_SUCESSO);
            }else{
                strcpy(conf,"ERRO");
            }


            sprintf(pipe,PIPE_CLIENTE,usr_tmp.nome);
            int fd_cli = open(pipe,O_WRONLY);
            
            if(fd_cli == -1){
                printf("Erro a abrir pipe utilizador.");
                continue;
            }

            write(fd_cli,conf,strlen(conf));
            close(fd_cli);
            // provavelmente isto vai sair daqui
            switch (res){
                case 0:
                    printf("Utilizador %s adicionado com sucesso.\n", usr_tmp.nome);
                    break;
                case 2:
                    printf("Ja existe um utilizador com esse nome.\n");
                    break;
                case 3:
                    printf("Ja ta cheio.\n");
                    break;
                default:
                    printf("Erro ao adicionar utilizador.\n");
                    break;
            }
        }
        close(fd_ler);
    } 
    else{
        close(fd_ler);
        char buffer[MAX_STR];

        while(loop){
            printf("\nAdmin> ");
    
            if(fgets(buffer,sizeof(buffer),stdin) == NULL){
                if (errno == EINTR){
                    continue;
                }
                // nao sei tem de ser isto po causo ctrl+D se nao parte se todo
                printf("EOF detectado. A terminar...\n");
                loop = 0; 
                continue;     
            }

            processar_comando_admin(buffer);
    }
    // isto por agora nao faz sentido mas depois com o comando termianr a funcionar acho que vai ser preciso
    union sigval valor_sinal; 
    valor_sinal.sival_int = 0; 
    sigqueue(pid_filho, SIGINT, valor_sinal); 

    wait(NULL); 
    unlink(PIPE_CONTROLADOR);
    }
    return 0;
};