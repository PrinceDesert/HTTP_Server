#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "../inc/adresse_internet.h"
#include "../inc/socket_tcp.h"
#include "../inc/config.h"

/**
 * Fichier server.c
 * Todo : timeout client et server en émission et réception
 * EN TCP : mettre un select sur le descripteur de fichier voir cours
 * utilisé setsockopt ?
 * ressources : 
  * https://code.tutsplus.com/tutorials/http-headers-for-dummies--net-8039
  * https://github.com/AaronKalair/C-Web-Server
*/

pid_t pid;
socket_tcp *s;

int main() {
	pid = getpid();
	printf("==================================================\n");
	printf("              Lancement du server %d              \n", pid);
	printf("==================================================\n");
	
	s = init_socket_tcp();
	if (s == NULL) {
		return EXIT_FAILURE;
	}
	// Juste pour récupèrer le domaine définit dans adresse_internet(AF_INET ou AF_INET6)
	adresse_internet *tmp = adresse_internet_new(ADDRESS, PORT);
	if ((s->socket_fd = socket(adresse_internet_get_domain(tmp), SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "[Erreur] création de la socket\n");
	}
	adresse_internet_free(tmp);
	printf("[Serveur:%d] Création de la socket %d\n", pid, s->socket_fd);
	
	if (ajoute_ecoute_socket_tcp(s, ADDRESS, PORT) == -1) {
		return EXIT_FAILURE;
	}
	
	socket_tcp *service = init_socket_tcp();
	printf("[Serveur:%d] En attente de client(s)\n", pid);
	
	int errnum;
	char buffer_read[BUFFER_SIZE];
	ssize_t n;
	while ((errnum = accept_socket_tcp(s, service)) != -1) {
		printf("[Serveur:%d] Allocation d'une ressource (thread)\n", pid);
		
		// mettre un timeout  select pour receive mais dans la fonction recev de tcp mais ici direct
		if ((n = read_socket_tcp(service, buffer_read, sizeof(buffer_read))) == -1) {
			fprintf(stderr, "[Erreur] read_socket_tcp %ld\n", n);
			break;
		}
		printf("[Server:%d] réception : %s\n", pid, buffer_read);
		
		// printf("[Server:%d] émission : %s\n", pid, msg);
		// write
	}
	if (errnum == -1) {
		return EXIT_FAILURE;
	}
	
	if (close_socket_tcp(s) == -1) {
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
