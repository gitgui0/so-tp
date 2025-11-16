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

// --- Constantes ---
#define MAX_USERS 30
#define MAX_VEICULOS 10
#define MAX_STR 100
#define MAX_PIPE MAX_STR + 15

#define PIPE_CONTROLADOR "/tmp/controlador_in"
#define PIPE_CLIENTE "/tmp/cliente_%s"

#define LOGIN_SUCESSO "OK"


// --- Structs ---
typedef struct User{
    char nome[MAX_STR];
    // char pipe[MAX_PIPE];
} User;


