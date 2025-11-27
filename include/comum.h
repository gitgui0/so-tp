#pragma once // Evita inclusões múltiplas

#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 700 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/select.h>

// --- Constantes ---
#define MAX_USERS 30
#define MAX_VEICULOS 10
#define MAX_STR 100
#define MAX_PIPE MAX_STR + 15

// Deixei assi talvez depois tira se nao sei 
#define MAX_SERVICOS 50

#define PIPE_CONTROLADOR "/tmp/controlador_in"
#define PIPE_CLIENTE "/tmp/cliente_%s"

#define LOGIN_SUCESSO "OK"

// --- Estados do Serviço ---
#define SERV_AGENDADO 0
#define SERV_EM_CURSO 1
#define SERV_CONCLUIDO 2

// --- Estados do Veículo ---
#define VEICULO_LIVRE 0
#define VEICULO_OCUPADO 1


// --- Structs ---
typedef struct User{
    char nome[MAX_STR];
    char fifo_privado[MAX_PIPE];
    int fd;                     
    pid_t pid_cliente;
    int ativo; // adicionei iusot pa depois facilitar a remoçao eadiçãop de novos users
} User;

typedef struct Servico{
    int id;
    int hora_agendada;
    int distancia;
    char origem[MAX_STR];
    int estado; // 0 - agendado, 1 - em curso, 2 - concluido
    pid_t pid_cliente;
    pid_t pid_veiculo;
} Servico;

typedef struct Veiculo{
    int id_servico;
    int estado;
    pid_t pid_veiculo;
    int fd_leitura;
} Veiculo;

typedef struct TUserInfo {
    int fd_pipe;                            // FD de leitura do PIPE_CONTROLADOR
    volatile int *loop_ptr;                 // Ponteiro para a variável global 'loop'
    pthread_mutex_t *users_mutex_ptr;       // Ponteiro para o Mutex que protege users/nUsers
} TUserInfo;

typedef struct TControlInfo {
    volatile int *loop_ptr; 
    pthread_mutex_t *servicos_mutex_ptr;
    /// mais coisas provavelmente
} TControlInfo;



// --- Funções ----
void listaUsers();
int existeEAdicionaUser(char* nome, char* fifo, pid_t pid);

