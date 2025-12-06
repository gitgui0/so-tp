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
#define VIAGEM_CONCLUIDA "VIAGEM_CONCLUIDA"



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
    int estado; // 0 - livre, 1 - ocupado
    pid_t pid_veiculo;
    int fd_leitura;
    int distancia;
    int distancia_percorrida;
} Veiculo;


typedef struct {
    // Variáveis de Controlo
    volatile int loop;
    volatile int tempo;
    int id_servico_atual;
    int max_veiculos;


    // Threads ids
    pthread_t control_tid;
    pthread_t clientes_tid;
    pthread_t tempo_tid;
    pthread_t frota_tid;

    // Dados
    User users[MAX_USERS];
    int nUsers;
    
    Servico servicos[MAX_SERVICOS];
    int nServicos;
    
    Veiculo frota[MAX_VEICULOS];
    int nVeiculos;

    // Mutexes
    pthread_mutex_t users_mutex;
    pthread_mutex_t servicos_mutex;
    pthread_mutex_t frota_mutex;
    
    // Pipe
    int fd_pipe_controlador;

} SistemaControlador;

typedef struct {
    int loop;
    int em_viagem;
    int pedido_terminar;
} ClienteEstado;

// --- Funções ----
void listaUsers(SistemaControlador *sys);
int existeEAdicionaUser(char* nome, char* fifo, pid_t pid,SistemaControlador *sys);

