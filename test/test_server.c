#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <socket_tcp.h>
#include <adresse_internet.h>
#include <config.h>

int main() {
	printf("Lancement du serveur %d\n", getpid());
	socket_tcp *s = init_socket_tcp();
	if (s == NULL) {
		fprintf(stderr, "Erreur init_socket_tcp\n");
		return EXIT_FAILURE;
	}
	printf("Initialisation de la socket\n");
	adresse_internet *tmp = adresse_internet_new(ADDRESS, PORT);
	s->socket_fd = socket(adresse_internet_get_domain(tmp), SOCK_STREAM, 0);
	adresse_internet_free(tmp);
	int reuse = 1;
	if (setsockopt(s->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
		perror("setsockopt");
		return EXIT_FAILURE;
	}
	if (ajoute_ecoute_socket_tcp(s, ADDRESS, PORT) == -1) {
		fprintf(stderr, "Erreur ajoute_ecoute_socket_tcp\n");
		return EXIT_FAILURE;
	}
	socket_tcp *service = init_socket_tcp();
	accept_socket_tcp(s, service);
	char buffer_read[1024];
	read_socket_tcp(service, buffer_read, sizeof(buffer_read));
	buffer_read[strlen(buffer_read)] = '\0';
	printf("Lecture de : %s\n", buffer_read);
	write_socket_tcp(service, buffer_read, sizeof(char) * (strlen(buffer_read) + 1));	
	printf("Fermeture de la socket de service %d\n", service->socket_fd);
	close_socket_tcp(service);
	printf("Fermeture de la socket %d\n", s->socket_fd);
	if (close_socket_tcp(s) == -1) {
		fprintf(stderr, "Erreur close_socket_tcp\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
