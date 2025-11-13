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

// --- Constantes ---
#define MAX_USERS 30
#define MAX_VEICULOS 10

#define PIPE_CONTROLADOR "/tmp/controlador_in"
#define PIPE_CLIENTE "/tmp/cliente_%d"


// --- Structs ---

