#include "comum.h"

int loop=1;

void handleSinal(int sinal, siginfo_t *info, void *context){
    if(sinal == SIGINT){
        printf("\nSIGINT\n");
        loop = 0;
    }
}

int executaComando(char* comando, User* user){
    if(strcmp(comando,"LOGIN") == 0 && user->ativo == 0){
        char login[150], confirmacao[5];
        int nbytes;

        //ABERTURA DO PIPE DO CONTROLADOR

        int fd_ctrl = open(PIPE_CONTROLADOR,O_WRONLY);
        if(fd_ctrl == -1){
            perror("Erro ao abrir pipe do controlador a partir do cliente.");
            exit(EXIT_FAILURE);
        }

        //comando a enviar
        sprintf(login, "LOGIN %s %s %d", user->nome, user->fifo_privado, getpid());
        printf("A enviar: %s\n",login);

        nbytes = write(fd_ctrl,&login,strlen(login));
        close(fd_ctrl);
        if(nbytes == -1){
            perror("Erro ao dar login com o cliente.");
            close(user->fd);
            unlink(user->fifo_privado);
            exit(EXIT_FAILURE);
        }

        //RECEBER CONFIRMACAO DO LOGIN DO CONTROLADOR

        nbytes = read(user->fd,&confirmacao,sizeof(confirmacao));
        
        if(nbytes > 0){
            confirmacao[nbytes] = '\0';
            confirmacao[strcspn(confirmacao, "\n")] = 0;

            if(strcmp(confirmacao, LOGIN_SUCESSO) == 0){ 
                printf("Login efetuado com sucesso!\n");
                user->ativo = 1;
            } else {
                printf("[ERRO] Login recusado pelo servidor. Resposta: '%s'\n", confirmacao);
                close(user->fd);
                unlink(user->fifo_privado);
                exit(EXIT_FAILURE); 
            }
        } else {
            perror("[ERRO] Falha ao ler resposta ou EOF");
            close(user->fd);
            unlink(user->fifo_privado);
            exit(EXIT_FAILURE);
        }
        
        
    }else{
        printf("Comando desconhecido.\n");
    }
    return 0;
}


int main(int argc, char* argv[]){

    
    User user;

    if(argc!=2){
        printf("Numero de argumentos errrado.\n O correto seria ./cliente pedro\n");
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

    user.fd = fd;    
    // LOGIN
    executaComando("LOGIN",&user);
    
    while(loop){
        //
        break;
    }

    close(fd);
    unlink(user.fifo_privado);

    return 0;
}