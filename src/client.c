#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "../inc/socket_tcp.h"
#include "../inc/config.h"

int main() {
	pid_t pid = getpid();
	printf("==================================================");
	printf("==            Lancement du client %d            ==\n", pid);
	printf("==================================================");
	socket_tcp *s = init_socket_tcp();
	if (s == NULL) {
		return EXIT_FAILURE;
	}
	if (connect_socket_tcp(s, ADDRESS, PORT) == -1) {
		return EXIT_FAILURE;
	}
	printf("[Client:%d] connexion à %s:%d effectué\n", pid, ADDRESS, PORT);
	
	const char *msg = "ping";
	if (write_socket_tcp(s, msg, sizeof(char) * strlen(msg) + 1) == -1) {
		fprintf(stderr, "[Erreur] write_socket_tcp\n");
		return EXIT_FAILURE;
	}
	printf("[Client:%d] émission : %s\n", pid, msg);
	char buffer_read[BUFFER_SIZE];
	if (read_socket_tcp(s, buffer_read, sizeof(buffer_read)) == -1) {
		fprintf(stderr, "[Erreur] read_socket_tcp\n");
	}
	printf("[Client:%d] réception : %s\n", pid, buffer_read);
	
	printf("[Client:%d] fermeture de la socket %d de connexion\n", pid, s->socket_fd);
	if (close_socket_tcp(s) == -1) {
		fprintf(stderr, "[Erreur] close_socket_tcp\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
