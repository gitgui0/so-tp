#include "comum.h"

int main(int argc, char* argv[]){
    
    if(argc != 6){
        printf("Numero de parametros errado.\nUso: ./veiculo <id> <origem> <distanciaTotal> <pipe_controlador> <pipe_cliente>");
        return 1;
    }

    int id = atoi(argv[1]);
    char* origem = argv[2];
    int distanciaTotal = atoi(argv[3]);
    int fd_ctrl = atoi(argv[4]);
    char* pipe_cliente = argv[5];

    pid_t pid = getpid();

    
    int fd_cli = open(pipe_cliente, O_WRONLY);
    if(fd_cli == -1){
        perror("Erro ao abrir pipe do cliente");
        return 1;
    }
    
    char msg_ctrl[MAX_STR];
    char msg_cli[MAX_STR];
    int distanciaPercorrida = 0;

    sprintf(msg_cli, "Veiculo %d iniciado para servico %d. Origem %s.\n", pid,id,origem); 
    write(fd_cli, msg_cli, strlen(msg_cli));

    while(distanciaPercorrida < distanciaTotal){
        sprintf(msg_cli, "Distancia: %d km de %d km\n", distanciaPercorrida, distanciaTotal);
        write(fd_cli, msg_cli, strlen(msg_cli));
        sprintf(msg_ctrl, "PROGRESSO %d %d", distanciaPercorrida, distanciaTotal);
        write(fd_ctrl, msg_ctrl, strlen(msg_ctrl));
        sleep(1);
        distanciaPercorrida++;
    }

    sprintf(msg_cli, "Veiculo %d chegou ao destino.\n", getppid());
    write(fd_cli, msg_cli, strlen(msg_cli));
    sprintf(msg_ctrl, "%s", VIAGEM_CONCLUIDA);
    write(fd_ctrl, msg_ctrl, strlen(msg_ctrl));


    return 0;
}