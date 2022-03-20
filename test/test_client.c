#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <socket_tcp.h>
#include <adresse_internet.h>
#include <config.h>

int main() {
	printf("Lancement du client %d\n", getpid());
	socket_tcp *s = init_socket_tcp();
	if (s == NULL) {
		fprintf(stderr, "Erreur init_socket_tcp\n");
		return EXIT_FAILURE;
	}
	printf("Initialisation de la socket\n");
	adresse_internet *tmp = adresse_internet_new(ADDRESS, PORT);
	s->socket_fd = socket(adresse_internet_get_domain(tmp), SOCK_STREAM, 0);
	adresse_internet_free(tmp);
	printf("Connexion vers %s:%d\n", ADDRESS, PORT);
	if (connect_socket_tcp(s, ADDRESS, PORT) == -1) {
		fprintf(stderr, "Erreur connect_socket_tcp\n");
		return EXIT_FAILURE;
	}
	const char *msg = "ping";
	write_socket_tcp(s, msg, sizeof(char) * (strlen(msg) + 1));
	char buffer_read[1024];
	read_socket_tcp(s, buffer_read, sizeof(buffer_read));
	buffer_read[strlen(buffer_read)] = '\0';
	printf("Lecture : %s\n", buffer_read);
	printf("Fermeture de la socket %d de connexion\n", s->socket_fd);
	if (close_socket_tcp(s) == -1) {
		fprintf(stderr, "Erreur close_socket_tcp\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
