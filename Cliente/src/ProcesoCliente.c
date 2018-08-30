#include "ProcesoCliente.h"

int main(int argc, char *argv[]) {
	system("clear");
	puts("PROCESO CLIENTE\n");

	log_cliente = init_log(PATH_LOG, "Proceso Cliente", true, LOG_LEVEL_INFO);
	log_consola = init_log(PATH_LOG, "Consola Cliente", false, LOG_LEVEL_INFO);

	log_info(log_cliente, "Inicio del proceso");

	if (argc != 2) {
		log_error(log_cliente, "Falta el parametro <Alias> del cliente");
		exit(EXIT_FAILURE);
	}

	config = load_config();

	servidor = connect_server(config.IP_SERVIDOR, config.PUERTO_SERVIDOR, log_cliente);

	alias = strdup(argv[1]);

	if (!handshake_servidor()) {
		log_error(log_cliente, "handshake");
		exit(EXIT_FAILURE);
	}

	pthread_create(&thread_input, NULL, (void *) input, NULL);
	pthread_create(&thread_output, NULL, (void *) output, NULL);

	pthread_join(thread_input, NULL);
	pthread_join(thread_output, NULL);

	exit(EXIT_SUCCESS);
}

config_t load_config() {
	t_config *config = config_create(PATH_CONFIG);

	config_t miConfig;
	miConfig.IP_SERVIDOR = strdup(config_get_string_value(config, "IP_SERVIDOR"));
	miConfig.PUERTO_SERVIDOR = config_get_int_value(config, "PUERTO_SERVIDOR");

	log_info(log_cliente, "---- Configuracion ----");
	log_info(log_cliente, "IP_SERVIDOR = %s", miConfig.IP_SERVIDOR);
	log_info(log_cliente, "PUERTO_SERVIDOR = %d", miConfig.PUERTO_SERVIDOR);
	log_info(log_cliente, "-----------------------");

	config_destroy(config);
	return miConfig;
}

bool handshake_servidor() {
	uint32_t hs_s = HANDSHAKE_SERVIDOR;
	bool rta;

	if (send_int(servidor, hs_s) == -1)
		return false;

	if (send_string(servidor, alias) == -1)
		return false;

	free(alias);

	if (receive_int(servidor, &rta) <= 0)
		return false;

	return rta;
}

void input() {
	char *linea;
	uint32_t eat = ENVIAR_A_TODOS;

	while (true) {
		linea = readline("");

		if (strlen(linea) > 0) {
			add_history(linea);

			if (send_int(servidor, eat) == -1)
				log_error(log_consola, "send (enviar a todos)");
			else
				if (send_string(servidor, linea) == -1)
					log_error(log_consola, "send (mensaje a enviar)");
		}
		free(linea);
	}
}

void output() {
	char *buf;

	while (true) {
		if (receive_string(servidor, &buf) <= 0)
			log_error(log_consola, "recv (mensaje recibido)");
		else
			print_c((void *) log_info, "%s", buf);

		free(buf);
	}
}

void print_c(void (*log_function)(t_log *, const char *), char *message_template, ...) {
	va_list arguments;
	va_start(arguments, message_template);
	char *message = string_from_vformat(message_template, arguments);
	va_end(arguments);
	log_function(log_consola, message);
	printf("%s\n", message);
	free(message);
}
