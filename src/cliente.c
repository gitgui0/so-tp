#include "comum.h"

int loop=1;
int em_viagem = 0; // 0 = Livre, 1 = Em viagem
int pedido_terminar = 0;


void handleSinal(int sinal, siginfo_t *info, void *context){
    if(sinal == SIGINT){
        printf("\nSIGINT\n");
        loop = 0;
    }
}

int main(int argc, char* argv[]){
    
    setbuf(stdout,NULL);

    User user;
    char buffer_teclado[MAX_STR];
    char buffer_pipe[MAX_STR*2];
    char msg_envio[MAX_STR*2];

    if(argc!=2){
        printf("Numero de argumentos errado.\n O correto seria ./cliente pedro\n");
        return 1;
    }

    if(access(PIPE_CONTROLADOR,F_OK) != 0){
        printf("Controlador nao esta a correr. \n");
        return 0;
    }

    //SINAIS 
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));

    sa.sa_sigaction = handleSinal;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    
    //INICIALIZACAO DO USER
    memset(&user,0,sizeof(User));
    strcpy(user.nome,argv[1]);
    sprintf(user.fifo_privado,PIPE_CLIENTE,user.nome);
    user.pid_cliente = getpid();
    user.ativo = 0;


    //CRIACAO DO PIPE PRIVADO DO CLIENTE
  

    if(mkfifo(user.fifo_privado,0666) == -1){ // 0666 por agora
        perror("Erro na criacao do pipe cliente");
        exit(EXIT_FAILURE);
    }

    int fd = open(user.fifo_privado,O_RDWR); // RDWR so para nao bloquear
    if(fd == -1){
        perror("Erro ao abrir pipe cliente.");
        unlink(user.fifo_privado);
        exit(EXIT_FAILURE);
    }

    // LOGIN
    int fd_ctrl = open(PIPE_CONTROLADOR, O_WRONLY);
    if(fd_ctrl == -1){
        perror("Erro ao abrir pipe controlador");
        close(fd);
        unlink(user.fifo_privado);
        exit(1);
    }
    

    sprintf(msg_envio, "LOGIN %s %s %d", user.nome, user.fifo_privado, user.pid_cliente);
    write(fd_ctrl, msg_envio, strlen(msg_envio));


    // ESPERAR RESPOSTA DO CONTROLADOR
    int n = read(fd, buffer_pipe, sizeof(buffer_pipe));
    if(n > 0){
        buffer_pipe[n] = '\0';
        if(strncmp(buffer_pipe, LOGIN_SUCESSO, 2) == 0){
            printf("[SUCESSO] Login aceite!\n");
            user.ativo = 1;
        } else {
            printf("Login recusado: %s\n", buffer_pipe);
            close(fd); 
            close(fd_ctrl); 
            unlink(user.fifo_privado);
            return 1;
        }
    } else {
        printf("Erro ao receber resposta de login.\n");
         close(fd); 
            close(fd_ctrl); 
            unlink(user.fifo_privado);
            return 1;
    }

    

    fd_set read_fds;
    int max_fd = fd;

    printf("> ");

    while(loop){
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds); // le do teclado (cliente)
        FD_SET(fd, &read_fds); // le do pipe privado (controlador)
        

        int atividade = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if(atividade < 0 && errno != EINTR) break;
        if(!loop) break;

        // Mensagens do Controlador ou Veiculo
        if(FD_ISSET(fd, &read_fds)){
            int nbytes = read(fd, buffer_pipe, sizeof(buffer_pipe)-1);
            if(nbytes > 0){
                buffer_pipe[nbytes] = '\0';
                printf("\n[RECEBIDO]: %s\n> ", buffer_pipe);

                if(strstr(buffer_pipe, "iniciado para servico") != NULL){
                    em_viagem = 1;
                }
                else if(strstr(buffer_pipe, "chegou ao destino") != NULL || strstr(buffer_pipe, "cancelado") != NULL){
                    em_viagem = 0;
                        
                    // Se o user ja tinha pedido para sair, agora pode sair
                    if(pedido_terminar == 1){
                        printf("\nViagem concluida. A terminar cliente...\n");
                        loop = 0;
                    }
                }
            }
        }

        //Comandos do Utilizador
        if(FD_ISSET(STDIN_FILENO, &read_fds)){
            if(fgets(buffer_teclado, sizeof(buffer_teclado), stdin)){
                buffer_teclado[strcspn(buffer_teclado, "\n")] = 0; // Remove \n

                if(strcmp(buffer_teclado, "terminar") == 0){
                    if(em_viagem){
                        printf("[AVISO] Nao pode sair durante uma viagem!\n");
                        printf("O cliente terminara automaticamente quando a viagem acabar.\n");
                        pedido_terminar = 1;
                    } else {
                        loop = 0; // Sai imediatamente se estiver livre
                    }
                }
                else if(strlen(buffer_teclado) > 0){
                    char cmd[MAX_STR], args[MAX_STR];
                    args[0] = '\0';
                    int res = sscanf(buffer_teclado, "%s %[^\n]", cmd, args);

                    if(res >= 1 && strcmp(cmd,"LOGIN") !=0 && strcmp(cmd,"LOGOUT") !=0){
                        if(res == 1) sprintf(msg_envio, "%s %s", cmd, user.nome);
                        else sprintf(msg_envio, "%s %s %s", cmd, user.nome, args);
                        
                        write(fd_ctrl, msg_envio, strlen(msg_envio));
                    }
                } 
                printf("> ");
                fflush(stdout);
            }
        }
    }

    close(fd);
    unlink(user.fifo_privado);

    return 0;
}