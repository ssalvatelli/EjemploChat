#ifndef PROCESOSERVIDOR_H_
#define PROCESOSERVIDOR_H_
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
	#include "funciones/funciones.h"
	#include "commons/config.h"
	#include "commons/collections/list.h"
	#include "servidor/servidor.h"

	// constantes
	#define NUEVO_CLIENTE 1
	#define NUEVO_MENSAJE 2

	char *PATH_LOG = "/home/utnso/workspace/EjemploChat/Logs/logServidor.txt";
	char *PATH_CONFIG = "/home/utnso/workspace/EjemploChat/Servidor/config.txt";

	// estructuras
	typedef struct {
		uint32_t PUERTO_ESCUCHA;
		uint32_t CANT_ENTRADAS;
	} config_t;

	typedef struct {
		uint32_t socket;
		char *alias;
	} client_t;

	//variables
	t_log *log_servidor;
	config_t config;

	t_list *lista_clientes;

	pthread_t thread_servidor;

	uint32_t socket_global;

	// funciones
	config_t load_config();
	void server();
	void remover_informar_a_todos(uint32_t client, uint32_t fdmax, fd_set master, uint32_t servidor);
	bool buscar_socket(client_t *cliente);
	void enviar_a_todos(uint32_t fdmax, fd_set master, uint32_t servidor, uint32_t client, char *message, char *error_msg);
	void command_handler(uint32_t command, uint32_t client, uint32_t fdmax, fd_set master, uint32_t servidor);
	void nuevo_cliente(uint32_t client, uint32_t fdmax, fd_set master, uint32_t servidor);
	void listar_clientes_conectados(client_t *cliente);
	void nuevo_mensaje(uint32_t client, uint32_t fdmax, fd_set master, uint32_t servidor);
#endif
