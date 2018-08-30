#include "ProcesoServidor.h"

int main(void) {
	system("clear");
	puts("PROCESO SERVIDOR\n");

	log_servidor = init_log(PATH_LOG, "Proceso Servidor", true, LOG_LEVEL_INFO);

	log_info(log_servidor, "Inicio del proceso");

	config = load_config();

	lista_clientes = list_create();

	pthread_create(&thread_servidor, NULL, (void *) server, NULL);

	pthread_join(thread_servidor, NULL);

	exit(EXIT_SUCCESS);
}

config_t load_config() {
	t_config *config = config_create(PATH_CONFIG);

	config_t miConfig;
	miConfig.PUERTO_ESCUCHA = config_get_int_value(config, "PUERTO_ESCUCHA");
	miConfig.CANT_ENTRADAS = config_get_int_value(config, "CANT_ENTRADAS");

	log_info(log_servidor, "---- Configuracion ----");
	log_info(log_servidor, "PUERTO_ESCUCHA = %d", miConfig.PUERTO_ESCUCHA);
	log_info(log_servidor, "CANT_ENTRADAS = %d", miConfig.CANT_ENTRADAS);
	log_info(log_servidor, "-----------------------");

	config_destroy(config);
	return miConfig;
}

void server() {
	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	struct sockaddr_in remoteaddr; // dirección del cliente
	uint32_t fdmax; // número máximo de descriptores de fichero
	uint32_t newfd; // descriptor de socket de nueva conexión aceptada
	uint32_t command; // comando del cliente
	uint32_t nbytes;
	uint32_t addrlen;
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	// obtener socket a la escucha
	uint32_t servidor = build_server(config.PUERTO_ESCUCHA, config.CANT_ENTRADAS, log_servidor);

	// añadir listener al conjunto maestro
	FD_SET(servidor, &master);
	// seguir la pista del descriptor de fichero mayor
	fdmax = servidor; // por ahora es éste

	// bucle principal
	while (true) {
		read_fds = master; // cópialo
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			log_error(log_servidor, "select");
			exit(EXIT_FAILURE);
		}
		// explorar conexiones existentes en busca de datos que leer
		for (uint32_t i = 0; i <= fdmax; i++)
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == servidor) {
					// gestionar nuevas conexiones
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(servidor, (struct sockaddr *) &remoteaddr, &addrlen)) == -1)
						log_error(log_servidor, "accept");
					else {
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) // actualizar el máximo
							fdmax = newfd;
						log_info(log_servidor, "Nueva conexion desde %s en el socket %d", inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				}
				else
					// gestionar datos de un cliente
					if ((nbytes = receive_int(i, &command)) <= 0) {
						// error o conexión cerrada por el cliente
						if (nbytes == 0) {
							// conexión cerrada
							remover_informar_a_todos();
							log_info(log_servidor, "Socket %d colgado", i);
						}
						else
							log_error(log_servidor, "recv (comando del cliente)");

						close(i); // ¡Hasta luego!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					}
					else
						// tenemos datos de algún cliente
						command_handler(command, i, fdmax, master, servidor);
			} // if (FD_ISSET(i, &read_fds))
	} // while (true)
}

void remover_informar_a_todos(uint32_t client, uint32_t fdmax, fd_set master, uint32_t servidor) {
	socket_global = client;

	client_t *cliente_colgado = list_remove_by_condition(lista_clientes, (void *) buscar_socket);

	if (cliente_colgado != NULL) {
		char *message = malloc(sizeof(char) * (strlen(cliente_colgado->alias) + 15));

		if (message != NULL) {
			strcpy(message, cliente_colgado->alias);
			strcat(message, " se desconecto");

			enviar_a_todos(fdmax, master, servidor, client, message, "conexion cerrada");

			free(message);
		}
		free(cliente_colgado->alias);
		free(cliente_colgado);
	}
}

bool buscar_socket(client_t *cliente) {
	return cliente->socket == socket_global;
}

void enviar_a_todos(uint32_t fdmax, fd_set master, uint32_t servidor, uint32_t client, char *message, char *error_msg) {
	char *error;

	for (uint32_t j = 0; j <= fdmax; j++)
		// ¡enviar a todos!
		if (FD_ISSET(j, &master))
			// excepto al listener y a nosotros mismos
			if (j != servidor && j != client)
				if (send_string(j, message) == -1) {
					error = malloc(sizeof(char) * (strlen(error_msg) + 8));

					if (error != NULL) {
						strcpy(error, "send (");
						strcat(error, error_msg);
						strcat(error, ")");

						log_error(log_servidor, error);

						free(error);
					}
				}
}

void command_handler(uint32_t command, uint32_t client, uint32_t fdmax, fd_set master, uint32_t servidor) {
	switch (command) {
	case NUEVO_CLIENTE:
		nuevo_cliente(client, fdmax, master, servidor);
		break;

	case NUEVO_MENSAJE:
		nuevo_mensaje(client, fdmax, master, servidor);
		break;

	default:
		log_warning(log_servidor, "%d: Comando del cliente incorrecto", command);
	}
}

void nuevo_cliente(uint32_t client, uint32_t fdmax, fd_set master, uint32_t servidor) {
	char *buf;

	if (receive_string(client, &buf) <= 0)
		log_error(log_servidor, "recv (alias del cliente)");
	else {
		if (send_int(client, true) == -1)
			log_error(log_servidor, "send (handshake ok)");
		else {
			socket_global = client;

			list_iterate(lista_clientes, (void *) listar_clientes_conectados);

			client_t *nuevo_cliente = malloc(sizeof(client_t));

			if (nuevo_cliente != NULL) {
				nuevo_cliente->socket = client;
				nuevo_cliente->alias = strdup(buf);

				list_add(lista_clientes, nuevo_cliente);

				char *message = malloc(sizeof(char) * (strlen(buf) + 22));

				if (message != NULL) {
					strcpy(message, buf);
					strcat(message, " esta ahora conectado");

					enviar_a_todos(fdmax, master, servidor, client, message, "nuevo cliente");

					free(message);
				}
			}
		}
		free(buf);
	}
}

void listar_clientes_conectados(client_t *cliente) {
	char *message = malloc(sizeof(char) * (strlen(cliente->alias) + 16));

	if (message != NULL) {
		strcpy(message, cliente->alias);
		strcat(message, " esta conectado");

		if(send_string(socket_global, message) == -1)
			log_error(log_servidor, "send (listar clientes conectados)");

		free(message);
	}
}

void nuevo_mensaje(uint32_t client, uint32_t fdmax, fd_set master, uint32_t servidor) {
	char *buf;

	if (receive_string(client, &buf) <= 0)
		log_error(log_servidor, "recv (nuevo mensaje)");
	else {
		socket_global = client;

		client_t *cliente_actual = list_find(lista_clientes, (void *) buscar_socket);

		if (cliente_actual != NULL) {
			char *message = malloc(sizeof(char) * (strlen(cliente_actual->alias) + strlen(buf) + 8));

			if (message != NULL) {
				strcpy(message, cliente_actual->alias);
				strcat(message, " dice: ");
				strcat(message, buf);

				enviar_a_todos(fdmax, master, servidor, client, message, "nuevo mensaje");

				free(message);
			}
		}
		free(buf);
	}
}
