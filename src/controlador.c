#include "comum.h"

int loop=1;

void handleSinal(int sinal, siginfo_t *info, void *context){
    if(sinal == SIGINT){
        printf("\nSIGINT\n");
        loop = 0;
    }

}


void listaUsers(User* users, int nUsers){
    printf("LISTA USERS\n");
    for(int i = 0; i < nUsers; i++){
        printf("%d. %s\n",i+1,users[i].nome);
    }
    printf("FIM LISTA USERS\n");
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

int main(){
    User usr_tmp;
    User users[MAX_USERS];
    int nUsers = 0, nbytes;
    char pipe[150], conf[5];
    
    setbuf(stdout,NULL);

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));

    sa.sa_sigaction = handleSinal;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    

    // pipe ja existe
    if(access(PIPE_CONTROLADOR,F_OK) == 0){
        perror("Ja existe um processo controlador ativo.");
        return 0;
    }

    if(mkfifo("/tmp/controlador_in", 0666) == -1){  // 0666 por agora
        perror("Erro ao criar pipe controlador.");
        exit(EXIT_FAILURE);
    }

    int fd_ler = open(PIPE_CONTROLADOR, O_RDWR);
    if(fd_ler == -1){
        perror("Erro a abrir pipe controlador.");
        unlink(PIPE_CONTROLADOR);
        exit(EXIT_FAILURE);
    }

    while(loop){
        printf("\n");
        memset(&usr_tmp, 0, sizeof(struct User));
        nbytes = read(fd_ler, usr_tmp.nome, sizeof(struct User));
        if(nbytes == -1){
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
        
        // provavelmente isto vai sair daqui
        switch (res){
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
        listaUsers(users,nUsers);
    }


    close(fd_ler);
    unlink(PIPE_CONTROLADOR);

    return 0;
}