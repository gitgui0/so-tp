#include "comum.h"


SistemaControlador *ptr_sistema = NULL;

User* devolveUserPorNome(SistemaControlador *sys,char* nome){
    for(int i = 0; i < sys->nUsers; i++){
        if(strcmp(sys->users[i].nome, nome) == 0){
            return &sys->users[i];
        }
    }
    return NULL;
}

User* devolveUserPorPID(SistemaControlador *sys,pid_t pid){
    for(int i = 0; i < sys->nUsers; i++){
        if(sys->users[i].pid_cliente == pid){
            return &sys->users[i];
        }
    }
    return NULL;
}

Servico* devolveServicoPorIDS(SistemaControlador *sys,int id){
    for(int i = 0; i < sys->nUsers; i++){
        if(sys->servicos[i].id == id){
            return &sys->servicos[i];
        }
    }
    return NULL;
}

Servico* devolveServicoPorUserID(SistemaControlador *sys,pid_t pidUser){
    for(int i = 0; i < sys->nUsers; i++){
        if(sys->servicos[i].pid_cliente == pidUser){
            return &sys->servicos[i];
        }
    }
    return NULL;
}

Veiculo* devolveVeiculoPorPID(SistemaControlador *sys,pid_t pid){
    for(int i = 0; i < sys->nVeiculos; i++){
        if(sys->frota[i].pid_veiculo == pid){
            return &sys->frota[i];
        }
    }
    return NULL;
}
void cancelarServico(SistemaControlador *sys,int idCancelar){
    pthread_mutex_lock(&sys->servicos_mutex);
    fflush(stdout); // Força a escrita no terminal

    int i = 0;
    while(i < sys->nServicos){    
        if(sys->servicos[i].id == idCancelar || idCancelar == 0){   
          
            // tratar do veiculo em servicos em curso
            if(sys->servicos[i].estado == SERV_EM_CURSO){
                pthread_mutex_lock(&sys->frota_mutex);
                fflush(stdout);
                int idx_veiculo = -1;
                for(int v=0; v<sys->nVeiculos; v++){
                    if(sys->frota[v].pid_veiculo == sys->servicos[i].pid_veiculo){
                        idx_veiculo = v;
                        break;
                    }
                }

                // encontrou o veiculo
                if(idx_veiculo != -1){
                    pid_t pid = sys->frota[idx_veiculo].pid_veiculo;
                    
                    kill(pid, SIGUSR1);
                    close(sys->frota[idx_veiculo].fd_leitura);

                    // remove da frota
                    sys->frota[idx_veiculo] = sys->frota[sys->nVeiculos - 1];
                    sys->nVeiculos--;
                    pthread_mutex_unlock(&sys->frota_mutex);
                    fflush(stdout);

                    waitpid(pid, NULL, 0);
                    
                } else {
                    // Se não encontrou veiculo (estranho, mas possivel), liberta lock
                    pthread_mutex_unlock(&sys->frota_mutex);
                }
                
            }

            printf("[CANCELAR] Servico ID %d cancelado.\n", sys->servicos[i].id);
            // remover o servico
            for(int j = i; j < sys->nServicos - 1; j++){
                sys->servicos[j] = sys->servicos[j+1];
            }
            sys->nServicos--; 
            if(idCancelar != 0){
                break; // se for para cancelar um especifico, sai aqui
            }
        } else {
            // so se avanca se nao removemos nada
            i++;
        }
    }
    
    pthread_mutex_unlock(&sys->servicos_mutex);
}

void listaUsers(SistemaControlador *sys){
    pthread_mutex_lock(&sys->users_mutex);
    printf("\n--- LISTA DE UTILIZADORES ---\n");
    for(int i = 0; i < sys->nUsers; i++){
        Servico *s = devolveServicoPorUserID(sys,sys->users[i].pid_cliente);
        char estado[MAX_STR];

        if(s==NULL) strcpy(estado, "Sem servico");
        else {
            if(s->estado == SERV_AGENDADO) strcpy(estado, "A espera de veiculo");
            else if(s->estado == SERV_EM_CURSO) strcpy(estado, "Em viagem");
        }

        printf("%d. %s (PID: %d) - %s\n", i+1, sys->users[i].nome, sys->users[i].pid_cliente,estado);
    }
    printf("-----------------------------\n");
    pthread_mutex_unlock(&sys->users_mutex);
}

void listaServicos(SistemaControlador *sys){
    pthread_mutex_lock(&sys->users_mutex);
    pthread_mutex_lock(&sys->servicos_mutex);
    printf("\n--- LISTA DE SERVICOS ---\n");
    for(int i = 0; i < sys->nServicos; i++){
        printf("(ID: %d) %d - %dkm -  %s - Estado: ",sys->servicos[i].id, sys->servicos[i].hora_agendada, sys->servicos[i].distancia, sys->servicos[i].origem);
        if(sys->servicos[i].estado == SERV_AGENDADO)
            printf("Agendado ");
        else if(sys->servicos[i].estado == SERV_EM_CURSO)
            printf("Em curso ");
        else if(sys->servicos[i].estado == SERV_CONCLUIDO)
            printf("Concluido ");
        printf("- Para %s (%d)\n",devolveUserPorPID(sys,sys->servicos[i].pid_cliente)->nome, sys->servicos[i].pid_cliente);
    }
    printf("-----------------------------\n");
    pthread_mutex_unlock(&sys->servicos_mutex);
    pthread_mutex_unlock(&sys->users_mutex);
}

void listaFrotaAtiva(SistemaControlador *sys){
    pthread_mutex_lock(&sys->frota_mutex);
    printf("\n--- LISTA DE VEICULOS ATIVOS ---\n");
    for(int i = 0; i < sys->nVeiculos; i++){
        if(sys->frota[i].estado == VEICULO_OCUPADO){
            printf("Veiculo(%d) - %.2f - %d km de %d km\n",
            sys->frota[i].pid_veiculo, (float)sys->frota[i].distancia_percorrida / sys->frota[i].distancia * 100,
            sys->frota[i].distancia_percorrida, sys->frota[i].distancia);
        }
    }
    printf("-----------------------------\n");
    pthread_mutex_unlock(&sys->frota_mutex);
}



// Return 0 - sucesso
// Return 1 - erro genérico
// Return 2 - já existe
// Return 3 - lista cheia
int existeEAdicionaUser(char* nome, char* fifo, pid_t pid,SistemaControlador *sys){
    if(sys->nUsers >= MAX_USERS) return 3;
    
    for(int i = 0; i < sys->nUsers; i++){
        if(strcmp(sys->users[i].nome, nome) == 0)
            return 2; // Já existe
    }

    // Adiciona novo utilizador
    strcpy(sys->users[sys->nUsers].nome, nome);
    strcpy(sys->users[sys->nUsers].fifo_privado, fifo); 
    sys->users[sys->nUsers].pid_cliente = pid;
    sys->users[sys->nUsers].ativo = 1;
    sys->nUsers++; // Incrementa o contador global
    return 0;
}


// -1 - nenhum veiculo livre
int devolveVeiculoLivre(SistemaControlador *sys){
    for(int i = 0; i < sys->nVeiculos; i++){
        if(sys->frota[i].estado == VEICULO_LIVRE){
            return i; 
        }
    }
    return -1; 
}

// Envia resposta sem bloquear
void enviaResposta(char* nome_user, char* msg,SistemaControlador *sys){
    char fifo[MAX_PIPE];
    fifo[0] = '\0';

    for(int i=0; i<sys->nUsers; i++){
        if(sys->users[i].ativo && strcmp(sys->users[i].nome, nome_user) == 0){
            strcpy(fifo, sys->users[i].fifo_privado);
            break;
        }
    }

    if(strlen(fifo) > 0){
        int fd = open(fifo, O_WRONLY); 
        if(fd != -1){
            write(fd, msg, strlen(msg));
            close(fd);
        }
    }
}


// --- TRATAMENTO DE COMANDOS ---
void tratarComandoCliente(char* cmd, char* nome, char* args,SistemaControlador *sys){ 
    
    if (strcmp(cmd, "LOGIN") == 0) {
        char fifo[MAX_PIPE];
        int pid;
        sscanf(args, "%s %d", fifo, &pid);
        
        int res = existeEAdicionaUser(nome, fifo, pid,sys);
        char resposta[MAX_STR];
        
        if(res == 0 || res == 2) strcpy(resposta, LOGIN_SUCESSO);
        else strcpy(resposta, "ERRO: Cheio");

        int fd_c = open(fifo, O_WRONLY);
        if(fd_c != -1){
            write(fd_c, resposta, strlen(resposta));
            close(fd_c);
        }
        printf("\n[LOGIN] User: %s", nome);

    }else if(strcmp(cmd,"LOGOUT") == 0){
        for(int i = 0; i < sys->nUsers; i++){
            if(strcmp(sys->users[i].nome, nome) == 0){
                printf("\n[LOGOUT] User: %s", nome);
                // Remover user
                for(int j = i; j < sys->nUsers - 1; j++){
                    sys->users[j] = sys->users[j + 1];
                }
                sys->nUsers--;

                for(int k = 0; k < sys->nServicos; k++){
                    if(sys->servicos[k].pid_cliente == sys->users[i].pid_cliente){
                        cancelarServico(sys,sys->servicos[k].id);
                        k--; 
                    }
                }
                break;
            }
        }
    }
    else if(strcmp(cmd,"agendar") == 0){
        Servico novo;
        memset(&novo, 0, sizeof(Servico));
        User* u = devolveUserPorNome(sys,nome);
        int fd_c = open(u->fifo_privado, O_WRONLY);
        char resposta[MAX_STR];
        
        //destino?
        sscanf(args, "%d %s %d", &novo.hora_agendada,  novo.origem, &novo.distancia);

        if(novo.hora_agendada < sys->tempo){
            sprintf(resposta, "Nao e possivel agendar um servico para uma hora ja passada.\n");
            write(fd_c, resposta, strlen(resposta));
            return;
        }

        novo.id = sys->id_servico_atual++;
        novo.estado = SERV_AGENDADO;
        novo.pid_cliente = devolveUserPorNome(sys,nome)->pid_cliente;
        novo.pid_veiculo = -1;
        sys->servicos[sys->nServicos] = novo;
        sys->nServicos++;
        sprintf(resposta, "Servico agendado com sucesso.\n");
        write(fd_c, resposta, strlen(resposta));

        

    }else if(strcmp(cmd,"consultar") == 0){
        char resposta[MAX_STR];

        User* u = devolveUserPorNome(sys,nome);

        int fd_c = open(u->fifo_privado, O_WRONLY);
        if(fd_c != -1){
            int encontrados=0;
            for(int i = 0; i < sys->nServicos; i++){
                if(sys->servicos[i].pid_cliente == u->pid_cliente){
                    sprintf(resposta, "Servico ID: %d - Hora: %d - Distancia: %d km - Origem: %s - Estado: ",
                        sys->servicos[i].id, sys->servicos[i].hora_agendada, sys->servicos[i].distancia, sys->servicos[i].origem);
                    if(sys->servicos[i].estado == SERV_AGENDADO)
                        strcat(resposta, "Agendado\n");
                    else if(sys->servicos[i].estado == SERV_EM_CURSO)
                        strcat(resposta, "Em curso\n");
                    else if(sys->servicos[i].estado == SERV_CONCLUIDO)
                        strcat(resposta, "Concluido\n");

                    write(fd_c, resposta, strlen(resposta));
                    encontrados++;
                }
            }
            if(encontrados==0){
                sprintf(resposta, "Nao foram encontrados servicos.\n");
                write(fd_c, resposta, strlen(resposta));
            }
        }else{
            printf("\nErro ao abrir pipe do cliente para consultar com o nome %s\n", nome);
        }

        
        
    }else if(strcmp(cmd, "cancelar") == 0){
        User* u = devolveUserPorNome(sys,nome);
        int id_input = -1;
        sscanf(args, "%d", &id_input);

        int ids_para_cancelar[MAX_SERVICOS];
        int count_cancelar = 0;

        // 1. IDENTIFICAR O QUE É PARA CANCELAR (COM LOCK)
        pthread_mutex_lock(&sys->servicos_mutex);
        
        for(int i = 0; i < sys->nServicos; i++){
            // Verifica se o serviço é deste cliente
            if(sys->servicos[i].pid_cliente == u->pid_cliente){
                // Verifica se é o ID pedido ou se é "cancelar todos" (0)
                if(id_input == 0 || sys->servicos[i].id == id_input){
                    ids_para_cancelar[count_cancelar] = sys->servicos[i].id;
                    count_cancelar++;
                }
            }
        }
        
        pthread_mutex_unlock(&sys->servicos_mutex);

        // 2. EXECUTAR O CANCELAMENTO (FORA DO LOCK DE LEITURA)
        // Agora podemos chamar cancelarServico com segurança, pois ela gere os seus próprios locks
        int eliminados = 0;
        for(int k = 0; k < count_cancelar; k++){
            cancelarServico(sys,ids_para_cancelar[k]);
            eliminados++;
        }

        // 3. RESPONDER AO CLIENTE
        int fd_c = open(u->fifo_privado, O_WRONLY);
        if(fd_c != -1){
            char resposta[MAX_STR];
            sprintf(resposta, "%d servico(s) cancelado(s).\n", eliminados);
            write(fd_c, resposta, strlen(resposta));
            close(fd_c);
        }
        }
    printf("\nAdmin> ");
    fflush(stdout);
}

void handleSinal(int sinal, siginfo_t *info, void *context){
    (void)info; (void)context; // Silenciar warnings de variaveis nao usadas
    if(sinal == SIGINT){
        printf("\n[DEBUG] Recebi SIGINT. A parar loop...\n");
        ptr_sistema->loop = 0; 
    }
    if(sinal == SIGALRM){
        ptr_sistema->tempo++;
        alarm(1);
    }
}


// --- Processamento Comandos Admin (Executado pela thread Main) ---
void processar_comando_admin(SistemaControlador *sys,char* buffer) {
    char comando[MAX_STR];
    int id_cancelar = -1; 

    buffer[strcspn(buffer, "\n")] = 0; // Remove \n
    
    // Limpa variaveis
    memset(comando, 0, MAX_STR);
    
    int args = sscanf(buffer, "%s %d", comando, &id_cancelar);
    if (args <= 0) return;

    // CORREÇÃO THREADS: Acede diretamente à memória protegida


    if (strcmp(comando, "listar") == 0) {
        listaServicos(sys);
    }
    else if (strcmp(comando, "utiliz") == 0) {
        listaUsers(sys);
    }
    else if (strcmp(comando, "frota") == 0) {
        listaFrotaAtiva(sys);
    }
    else if (strcmp(comando, "km") == 0) {
        int count = 0;
        pthread_mutex_lock(&sys->frota_mutex);
        for(int i = 0; i < sys->nVeiculos; i++)
            if(sys->frota[i].estado == VEICULO_OCUPADO)
                count += sys->frota[i].distancia_percorrida;
        pthread_mutex_unlock(&sys->frota_mutex);
        printf("Distancia total percorrida por todos os veiculos: %d km\n", count);

    }
    else if (strcmp(comando, "hora") == 0) {
        printf("Tempo simulado: %d\n", sys->tempo);
    }
    else if (strcmp(comando, "cancelar") == 0 && args == 2) {
        cancelarServico(sys,id_cancelar);
    }
    else if (strcmp(comando, "terminar") == 0) {
        sys->loop = 0;
    }
    else {
        printf("Comando desconhecido: %s\n", comando);
    }
}


// --- Thread: Processamento de Clientes ---
void* tUsers(void* arg) {
    SistemaControlador *sys = (SistemaControlador*)arg;

    printf("\n[THREAD - CLIENTE] Thread cliente iniciada.\n");

    // Variáveis locais
    char buffer_msg[256];
    char cmd[20], nome[MAX_STR], args[MAX_STR * 2];

    while (sys->loop) { 
        int nbytes = read(sys->fd_pipe_controlador, buffer_msg, sizeof(buffer_msg) - 1);
        
        if (nbytes > 0) {
            buffer_msg[nbytes] = '\0';
            
            
            int res_scan = sscanf(buffer_msg, "%s %s %[^\n]", cmd, nome, args);
            printf("\n[THREAD-USER] MENSAGEM RECEBIDA: %s\n", buffer_msg);

            if(res_scan < 2) {
                printf("\nMsg invalida: %s\nAdmin> ", buffer_msg);
                continue;
            }

            // PROTEÇÃO (LOCK)
            pthread_mutex_lock(&sys->users_mutex);
            tratarComandoCliente(cmd, nome, args,sys);
            args[0] = '\0';
            // LIBERTAR (UNLOCK)
            pthread_mutex_unlock(&sys->users_mutex);    
            } 
        else if (nbytes == -1 && errno != EINTR) {
            perror("[ERRO] Leitura do FIFO principal");
            sleep(1);
        }
    }
    close(sys->fd_pipe_controlador);
    return NULL;
}

void* tControl(void* arg) {
    SistemaControlador *sys = (SistemaControlador*)arg;

    // Variáveis locais
    char buffer[MAX_STR];
    
    printf("\n[THREAD - CONTROL] Thread control iniciada.\n");
    fflush(stdout);

    while(sys->loop){
        printf("Admin> ");
        if(fgets(buffer, sizeof(buffer), stdin) == NULL){
            if (errno == EINTR) continue;
            sys->loop = 0; // EOF (Ctrl+D)
            break;     
        }
        processar_comando_admin(sys,buffer);

        // no caso de ter sido o comando termina
        if(sys->loop==0){
            break;
        }
    }
    
    printf("[THREAD] Thread control a terminar.\n");
    return NULL;
}

void* tTempo(void* arg){
    SistemaControlador *sys = (SistemaControlador*)arg;
    while(sys->loop){
        // so para testar
        // printf("\n%d", tempo);
        // Verificar para todos os servicos, se o tempo chegou a hora do servico
       
        
        for(int i = 0; i < sys->nServicos; i++){
            // <= ou == ?
            // meti <= so para estar seguro
            
            pthread_mutex_lock(&sys->servicos_mutex);
                fflush(stdout);
            if(sys->servicos[i].estado == SERV_CONCLUIDO){
                //shift
                for(int j = i + 1; j < sys->nServicos ; j++){
                    sys->servicos[j - 1] = sys->servicos[j];
                }
                sys->nServicos--;
                // Decrementamos i para não saltar o próximo elemento após o shift
                i--; 
                pthread_mutex_unlock(&sys->servicos_mutex);
                continue;
            }
            if(sys->servicos[i].estado == SERV_AGENDADO && sys->servicos[i].hora_agendada <= sys->tempo){  
                // Iniciar servico
                if(sys->nVeiculos >= sys->max_veiculos){
                    printf("Nao ha veiculos livres e ja atingimos o maximo de veiculos (%d)\n", sys->max_veiculos);
                    pthread_mutex_unlock(&sys->servicos_mutex); 
                    continue;
                } 
                
                int user_ocupado = 0;
                pid_t pid_cliente_atual = sys->servicos[i].pid_cliente;

                for(int k = 0; k < sys->nServicos; k++){
                    // Se encontrarmos um serviço DESTE cliente que já esteja EM CURSO
                    if(sys->servicos[k].pid_cliente == pid_cliente_atual && sys->servicos[k].estado == SERV_EM_CURSO){
                        user_ocupado = 1;
                        break;
                    }
                }

                if(user_ocupado){
                    pthread_mutex_unlock(&sys->servicos_mutex);
                    continue; // Salta para o próximo serviço, este fica à espera
                }
                
                Veiculo novo;
                memset(&novo, 0, sizeof(Veiculo));
                novo.id_servico = sys->servicos[i].id;
                novo.estado = VEICULO_OCUPADO;
                int fd_v[2];
                if(pipe(fd_v) == -1){
                    perror("Erro ao criar pipe veiculo-controlador");
                    continue;
                }
                pid_t pid = fork();
                if(pid == -1){
                    perror("Erro ao criar processo veiculo");
                    continue;
                }
                // Processo Filho - Veiculo
                if(pid== 0){
                    
                    // Argumentos
                    // ./veiculo <id> <origem> <distancia> <fifo_cliente>
                    close(fd_v[0]); // fecha leitura

                    close(STDOUT_FILENO);

                    if (dup(fd_v[1]) == -1) {
                        perror("Erro no dup");
                        exit(EXIT_FAILURE);
                    }

                    close(fd_v[1]);

                    char str_id[MAX_STR];
                    char str_distancia[MAX_STR];
                    char pipe[MAX_PIPE];

                    sprintf(str_id, "%d", sys->servicos[i].id);
                    sprintf(str_distancia, "%d",sys-> servicos[i].distancia);
                    User* u = devolveUserPorPID(sys,sys->servicos[i].pid_cliente);
                    if(u == NULL){
                        perror("[ERRO CRITICO] User do servico nao encontrado");
                        exit(EXIT_FAILURE);
                    }
                    sprintf(pipe, "%s", u->fifo_privado);
                    
                    execl("./veiculo", "veiculo", str_id, sys->servicos[i].origem, str_distancia, pipe, NULL);
                    
                    perror("[ERRO CRITICO] execl falhou");
                    exit(EXIT_FAILURE);
                }
                // Processo Pai - Controlador
                else{
                    close(fd_v[1]); // fecha escrita

                    int flags = fcntl(fd_v[0], F_GETFL, 0);
                    fcntl(fd_v[0], F_SETFL, flags | O_NONBLOCK);

                    pthread_mutex_lock(&sys->frota_mutex);

                    novo.id_servico = sys->servicos[i].id;
                    novo.estado = VEICULO_OCUPADO;
                    novo.pid_veiculo = pid;
                    novo.fd_leitura = fd_v[0];
                    novo.distancia = sys->servicos[i].distancia;
                    novo.distancia_percorrida = 0;
                    
                    
                    sys->frota[sys->nVeiculos] = novo;
                    sys->nVeiculos++;

                    pthread_mutex_unlock(&sys->frota_mutex);

                    
                    sys->servicos[i].pid_veiculo = pid;
                    sys->servicos[i].estado = SERV_EM_CURSO;
                    
                    User* u = devolveUserPorPID(sys,sys->servicos[i].pid_cliente);
                    printf(" Veiculo (%d) iniciado para %s(%d) \n", novo.pid_veiculo, u->nome, u->pid_cliente);
                    
                }
             }
              pthread_mutex_unlock(&sys->servicos_mutex);
           }
            sleep(1); // NAO TENHO A CERTEZA DISTO MAS ACHO QUE FAZ SENTIDO
        }
    return NULL;
}

void *tTFrota(void* arg){
    SistemaControlador *sys = (SistemaControlador*)arg;
    while(sys->loop){
        pthread_mutex_lock(&sys->frota_mutex);
        for(int i = 0; i < sys->nVeiculos; i++){
            if(sys->frota[i].estado == VEICULO_OCUPADO){
                char buf[MAX_STR];
                char msg[MAX_STR];
                int distanciaAtual, idServico = sys->frota[i].id_servico;

                Servico* s = devolveServicoPorIDS(sys,idServico);
                User* u = devolveUserPorPID(sys,s->pid_cliente);

                /// caso para mensagem de progresso
                int nbytes = read(sys->frota[i].fd_leitura, buf, sizeof(buf));
                
                if(nbytes > 0){
                    buf[nbytes] = '\0';

                    sscanf(buf, "%s %d", msg, &distanciaAtual);

                    if(strcmp(msg,VIAGEM_CONCLUIDA)==0){
                        pid_t pid_veiculo = sys->frota[i].pid_veiculo;
                        int id_servico = sys->frota[i].id_servico;

                        close(sys->frota[i].fd_leitura);
                        for(int j = i; j < sys->nVeiculos - 1; j++){
                            sys->frota[j] = sys->frota[j + 1];
                        }
                        sys->nVeiculos--;
                        i--; 

                        // --- IMPORTANTE: LIBERTAR A FROTA AGORA ---
                        pthread_mutex_unlock(&sys->frota_mutex);

                        printf("[THREAD-FROTA] Veiculo %d terminou viagem.\n", pid_veiculo);
 
                        pthread_mutex_lock(&sys->servicos_mutex);
                        if(s != NULL){
                            s->estado = SERV_CONCLUIDO;
                            // Opcional: obter nome do user (com cuidado ou cache)
                            printf("[THREAD-FROTA] Servico %d concluido.\n", id_servico);
                        }
                        pthread_mutex_unlock(&sys->servicos_mutex);

                        waitpid(pid_veiculo, NULL, 0);

                        // Voltar a bloquear para continuar o loop da frota
                        pthread_mutex_lock(&sys->frota_mutex);
                    }else if(strcmp(msg,"PROGRESSO")==0){
                        sys->frota[i].distancia_percorrida = distanciaAtual;  
                    } else{
                        printf("\n[THREAD-FROTA] [VEICULO %d para %s(%d)]: %s\n", sys->frota[i].pid_veiculo, u->nome, u->pid_cliente, msg);
                    }
                }
            }
        }
        pthread_mutex_unlock(&sys->frota_mutex);
        sleep(1); // nao e ideal mas nao sei outra solucao
    }
    return NULL;
}

// --- MAIN ---
int main(){
    setbuf(stdout,NULL);

    pthread_t control_tid;
    pthread_t clientes_tid;
    pthread_t tempo_tid;
    pthread_t frota_tid;

    SistemaControlador *sys = malloc(sizeof(SistemaControlador));
    if (!sys) { perror("Erro malloc"); return 1; }

    memset(sys, 0, sizeof(SistemaControlador));
    sys->loop = 1;
    sys->id_servico_atual = 1;

    ptr_sistema = sys;

    char* s_nveiculos = getenv("NVEICULOS");
    if(s_nveiculos == NULL) 
        sys->max_veiculos = MAX_VEICULOS; 
    else {
        sys->max_veiculos = atoi(s_nveiculos); 
        if (sys->max_veiculos <= 0) sys->max_veiculos = MAX_VEICULOS;
        if (sys->max_veiculos > MAX_VEICULOS) {
            printf("AVISO: NVEICULOS (%d) excede o limite. A usar %d.\n", sys->max_veiculos, MAX_VEICULOS);
            sys->max_veiculos = MAX_VEICULOS;
        }
    }
    printf("Max Veiculos = %d\n", sys->max_veiculos);

    pthread_mutex_init(&sys->users_mutex, NULL);
    pthread_mutex_init(&sys->servicos_mutex, NULL);
    pthread_mutex_init(&sys->frota_mutex, NULL);

    if(access(PIPE_CONTROLADOR,F_OK) == 0){
        printf("Ja existe um controlador a correr.\n");
        return 0;
    }

    // 2. Sinais
    struct sigaction sa_int;
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_sigaction = handleSinal;
    sa_int.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa_int, NULL);


    struct sigaction sa_alrm;
    memset(&sa_alrm, 0, sizeof(sa_alrm));
    sa_alrm.sa_sigaction = handleSinal;
    sa_alrm.sa_flags = SA_SIGINFO | SA_RESTART; 
    sigaction(SIGALRM, &sa_alrm, NULL);;

    // 3. Preparar FIFO
    unlink(PIPE_CONTROLADOR); 

    if(mkfifo(PIPE_CONTROLADOR, 0666) == -1){ 
        if (errno != EEXIST) {
            perror("Erro fatal ao criar FIFO");
            exit(EXIT_FAILURE);
        }
    }

    // Abrir FIFO (O_RDONLY bloqueia até um cliente abrir para escrita, 
    // ou usamos O_RDWR para manter aberto sem clientes - truque comum)
    sys->fd_pipe_controlador = open(PIPE_CONTROLADOR, O_RDWR); 
    if(sys->fd_pipe_controlador == -1){
        perror("Erro fatal ao abrir FIFO");
        unlink(PIPE_CONTROLADOR);
        exit(EXIT_FAILURE);
    };

    // 4. Iniciar Tempo e Mutex
    alarm(1);
    pthread_mutex_init(&sys->users_mutex, NULL);
    pthread_mutex_init(&sys->servicos_mutex, NULL);
    pthread_mutex_init(&sys->frota_mutex, NULL);

    if (pthread_create(&clientes_tid, NULL, tUsers, (void*)sys) != 0) {
        perror("Erro ao criar thread para clientes.");
        exit(1);
    }
    
    if (pthread_create(&control_tid, NULL, tControl, (void*)sys) != 0) {
        perror("Erro ao criar thread para controlo do controlador");
        exit(1);
    }

    if (pthread_create(&tempo_tid, NULL, tTempo, (void*)sys) != 0) {
        perror("Erro ao criar thread para controlo do controlador");
        exit(1);
    }

    if (pthread_create(&frota_tid, NULL, tTFrota, (void*)sys) != 0) {
        perror("Erro ao criar thread para controlo do controlador");
        exit(1);
    }

    // pthread_cancel so para o ctrl c
    pthread_join(control_tid, NULL);

    // Cancelar a thread de clientes (que está bloqueada a ler o named pipe)
    pthread_cancel(clientes_tid); 
    pthread_join(clientes_tid, NULL);

    pthread_join(tempo_tid,NULL);
    pthread_join(frota_tid,NULL);

    // Fechar a sessao dos clientes que ainda estavam ativos
    for(int i = 0; i < sys->nUsers; i++){
        if(sys->users[i].ativo){
            union sigval val;
            val.sival_int = 0;
            
            if(sigqueue(sys->users[i].pid_cliente, SIGINT, val) == -1){
                perror("Erro ao enviar sigqueue para cliente");
            }
        }
    }   
    
    // 3. Limpeza Final
    close(sys->fd_pipe_controlador);
    unlink(PIPE_CONTROLADOR);
    pthread_mutex_destroy(&sys->users_mutex);
    pthread_mutex_destroy(&sys->frota_mutex);
    pthread_mutex_destroy(&sys->servicos_mutex);

    free(sys);
    return 0;
} 