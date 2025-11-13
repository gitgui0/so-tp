#include "comum.h"

int loop=1;

void handleSinal(int sinal, siginfo_t *info, void *context){
    if(sinal == SIGINT){
        printf("\nSIGINT\n");
        loop = 0;
    }

}


int main(int argc, char* argv[]){

    char login[150], confirmacao[5];
    int nbytes;

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));

    sa.sa_sigaction = handleSinal;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    
    if(access(PIPE_CONTROLADOR,F_OK) != 0){
        printf("Controlador nao esta a correr. \n");
        return 0;
    }

    if(argc!=2){
        printf("Numero de argumentos errrado.\n O correto seria ./cliente pedro\n");
        return 1;
    }

    struct User user;
    memset(&user,0,sizeof(struct User));

    strcpy(user.nome,argv[1]);


    char pipe[MAX_PIPE];
    sprintf(pipe,PIPE_CLIENTE,user.nome);

    if(mkfifo(pipe,0666) == -1){ // 0666 por agora
        perror("Erro na criacao do pipe cliente");
        exit(EXIT_FAILURE);
    }

    int fd = open(pipe,O_RDWR);
    if(fd == -1){
        perror("Erro ao abrir pipe cliente.");
        exit(EXIT_FAILURE);
    }

    sprintf(login, "%s", user.nome);

    int fd_ctrl = open(PIPE_CONTROLADOR,O_WRONLY);
    if(fd_ctrl == -1){
        perror("Erro ao abrir pipe cliente.");
        unlink(pipe);
        exit(EXIT_FAILURE);
    }

    nbytes = write(fd_ctrl,&login,strlen(login));
    if(nbytes == -1){
        perror("Erro ao dar login com o cliente.");
        close(fd);
        unlink(pipe);
    }

    nbytes = read(fd,&confirmacao,sizeof(confirmacao));
    if(nbytes == -1 || strcmp(confirmacao,LOGIN_SUCESSO) != 0){
        perror("Erro ao dar login com o cliente.");
        close(fd);
        unlink(pipe);
    }
    

    printf("DO CONTROLADOR: %s\n", confirmacao);



    while(loop){
        //
        break;
    }

    close(fd);
    unlink(pipe);

    return 0;
}