#include "comum.h"

int main(){
    printf("Controlador arranca\n");

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

    char buf[100];
    read(fd_ler,buf,sizeof(buf));
    printf("%s", buf);

    close(fd_ler);
    unlink(PIPE_CONTROLADOR);

    return 0;
}