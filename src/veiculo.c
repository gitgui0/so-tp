#include "comum.h"

int main(int argc, char* argv[]){
    
    if(argc != 5){
        printf("Numero de parametros errado.\nUso: ./veiculo <id> <origem> <distancia> <fifo_cliente>");
        return 1;
    }

    printf("Veiculo a arrancar com estes parametros:\n");
    printf("ID: %s\n", argv[1]);
    printf("Origem: %s\n", argv[2]);
    printf("Distancia: %s\n", argv[3]);
    printf("FIFO Cliente: %s\n", argv[4]);

    return 0;
}