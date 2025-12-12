#include "comum.h"

volatile int *ptr_cancelar = NULL;

void handleSinal(int sinal, siginfo_t *info, void *context){
    (void)info; (void)context;
    if(sinal == SIGUSR1){
        *ptr_cancelar=1;
    }
}

int main(int argc, char* argv[]){

    volatile int cancelar = 0;
    
    ptr_cancelar = &cancelar;
    
    if(argc != 5){
        printf("Numero de parametros errado.\nUso correto: ./veiculo <id> <origem> <distanciaTotal> <pipe_cliente>");
        return 1;
    }

    int id = atoi(argv[1]);
    char* origem = argv[2];
    int distanciaTotal = atoi(argv[3]);
    char* pipe_cliente = argv[4];

    pid_t pid = getpid();

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));

    sa.sa_sigaction = handleSinal;
    sigaction(SIGUSR1, &sa, NULL);

    
    int fd_cli = open(pipe_cliente, O_WRONLY);
    if(fd_cli == -1){
        perror("Erro ao abrir pipe do cliente.");
        return 1;
    }
    
    char msg_ctrl[MAX_STR];
    char msg_cli[MAX_STR];
    int distanciaPercorrida = 0;

    sprintf(msg_cli, "Veiculo %d iniciado para servico %d. Origem %s.\n", pid,id,origem); 
    write(fd_cli, msg_cli, strlen(msg_cli));

    int passo_notificacao = (distanciaTotal + 10/2) / 10;
    if (passo_notificacao < 1) passo_notificacao = 1;

    while(distanciaPercorrida < distanciaTotal && !cancelar){
        if (distanciaPercorrida % passo_notificacao == 0 || distanciaPercorrida == 0) {
            sprintf(msg_cli, "Distancia: %d km de %d km\n", distanciaPercorrida, distanciaTotal);
            write(fd_cli, msg_cli, strlen(msg_cli));
            sprintf(msg_ctrl, "PROGRESSO %d %d", distanciaPercorrida, distanciaTotal);
            write(STDOUT_FILENO, msg_ctrl, strlen(msg_ctrl));
        }
        sleep(1); 
        
        if(!cancelar){
            distanciaPercorrida++;
        }
    }

    if(cancelar){
        sprintf(msg_cli, "O servico foi cancelado.\n");
        write(fd_cli, msg_cli, strlen(msg_cli));
    }else{
        sprintf(msg_cli, "Veiculo %d chegou ao destino.\n", getpid());
        write(fd_cli, msg_cli, strlen(msg_cli));
        sprintf(msg_ctrl, "%s", VIAGEM_CONCLUIDA);
        write(STDOUT_FILENO, msg_ctrl, strlen(msg_ctrl));
    }
        
    close(fd_cli);
    return 0;
}
