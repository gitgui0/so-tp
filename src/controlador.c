#include "comum.h"

int loop=1;

void handleSinal(int sinal, siginfo_t *info, void *context){
    if(sinal == SIGINT){
        printf("\nSIGINT\n");
        loop = 0;
    }

}


int main(){
    
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
        char buf[100];
        read(fd_ler,buf,sizeof(buf));
        printf("%s", buf);   
    }


    close(fd_ler);
    unlink(PIPE_CONTROLADOR);

    return 0;
}