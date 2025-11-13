#include "comum.h"

int loop=1;

void handleSinal(int sinal, siginfo_t *info, void *context){
    if(sinal == SIGINT){
        printf("\nSIGINT\n");
        loop = 0;
    }

}


int main(int argc, char* argv[]){

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

    strcpy(user.nome,argv[0]);
    sprintf(user.pipe,PIPE_CLIENTE,getpid());

    if(mkfifo(user.pipe,0666) == -1){ // 0666 por agora
        perror("Erro na criacao do pipe cliente");
        exit(EXIT_FAILURE);
    }

    int fd = open(user.pipe,O_RDWR);
    if(fd == -1){
        perror("Erro ao abrir pipe cliente.");
        unlink(user.pipe);
        exit(EXIT_FAILURE);
    }
    while(loop){
        scanf("%s",user.pipe);
    }

    close(fd);
    unlink(user.pipe);

    return 0;
}