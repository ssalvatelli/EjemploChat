#ifndef PROCESOCLIENTE_H_
#define PROCESOCLIENTE_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pthread.h>
	#include <readline/readline.h>
	#include <readline/history.h>
	#include <stdarg.h>
	#include "funciones/funciones.h"
	#include "commons/config.h"
	#include "servidor/servidor.h"
	#include "commons/string.h"

	// constantes
	#define HANDSHAKE_SERVIDOR 1
	#define ENVIAR_A_TODOS 2

	char *PATH_LOG = "/home/utnso/workspace/EjemploChat/Logs/logCliente.txt";
	char *PATH_CONFIG = "/home/utnso/workspace/EjemploChat/Cliente/config.txt";

	// estructuras
	typedef struct {
		char *IP_SERVIDOR;
		uint32_t PUERTO_SERVIDOR;
	} config_t;

	// variables
	t_log *log_cliente;
	t_log *log_consola;

	config_t config;

	uint32_t servidor;

	char *alias;

	pthread_t thread_input;
	pthread_t thread_output;

	// funciones
	config_t load_config();
	bool handshake_servidor();
	void input();
	void output();
	void print_c(void (*log_function)(t_log *, const char *), char *message_template, ...);
#endif
