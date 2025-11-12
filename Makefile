CC=gcc
CFLAGS=-Wall -Wextra -g -Iinclude # -Iinclude diz ao GCC para procurar ficheiros .h na pasta include

# Binários que queremos criar
EXECS = bin/controlador bin/cliente bin/veiculo

# Target 'all' compila tudo
all: $(EXECS)

# Regras para compilar cada programa
bin/controlador: src/controlador.c
	$(CC) $(CFLAGS) -o $@ $< 

bin/cliente: src/cliente.c
	$(CC) $(CFLAGS) -o $@ $<

bin/veiculo: src/veiculo.c
	$(CC) $(CFLAGS) -o $@ $<

# Target 'clean' apaga tudo
clean:
	rm -f $(EXECS) src/*.o *.o
