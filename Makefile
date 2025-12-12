CC=gcc
CFLAGS=-Wall -Wextra -g -Iinclude 

EXECS = controlador cliente veiculo

all: $(EXECS)

# Regras para compilar cada programa

controlador: src/controlador.c
	$(CC) $(CFLAGS) -o $@ $< -lpthread

cliente: src/cliente.c
	$(CC) $(CFLAGS) -o $@ $< -lpthread

veiculo: src/veiculo.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(EXECS) src/*.o *.o
