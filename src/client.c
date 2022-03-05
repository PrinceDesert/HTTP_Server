#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "../inc/adresse_internet.h"
#include "../inc/socket_tcp.h"
#include "../inc/config.h"

int main() {
	pid_t pid = getpid();
	printf("==================================================\n");
	printf("              Lancement du client %d             \n", pid);
	printf("==================================================\n");
	socket_tcp *s = init_socket_tcp();
	if (s == NULL) {
		return EXIT_FAILURE;
	}
	// Juste pour récupèrer le domaine définit dans adresse_internet (AF_INET ou AF_INET6)
	adresse_internet *tmp = adresse_internet_new(ADDRESS, PORT);
	if ((s->socket_fd = socket(adresse_internet_get_domain(tmp), SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "[Erreur] création de la socket\n");
	}
	adresse_internet_free(tmp);
	printf("[Serveur:%d] Création de la socket %d\n", pid, s->socket_fd);
	printf("[Client:%d] Connexion à %s:%d effectué\n", pid, ADDRESS, PORT);
	if (connect_socket_tcp(s, ADDRESS, PORT) == -1) {
		return EXIT_FAILURE;
	}
	
	const char *msg = "ping";
	if (write_socket_tcp(s, msg, sizeof(char) * strlen(msg) + 1) == -1) {
		fprintf(stderr, "[Erreur] write_socket_tcp\n");
		return EXIT_FAILURE;
	}
	printf("[Client:%d] émission : %s\n", pid, msg);
	char buffer_read[BUFFER_SIZE];
	buffer_read[BUFFER_SIZE - 1] = '\0';
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
