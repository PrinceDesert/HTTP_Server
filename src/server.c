#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "../inc/socket_tcp.h"
#include "../inc/config.h"

/**
 * Fichier server.c
 * Todo : timeout client et server en émission et réception
*/

socket_tcp *s;

int main() {
	pid_t pid = getpid();
	printf("==================================================");
	printf("==            Lancement du server %d            ==\n", pid);
	printf("==================================================");
	
	s = init_socket_tcp();
	if (s == NULL) {
		return EXIT_FAILURE;
	}
	
	if (ajoute_ecoute_socket_tcp(s, ADDRESS, PORT) == -1) {
		return EXIT_FAILURE;
	}
	
	socket_tcp *service = NULL;
	if (accept_socket_tcp(s, service) == -1) {
		return EXIT_FAILURE;
	}
	
	char buffer_read[BUFFER_SIZE];
	while (1) {
		// mettre un timeout pour receive mais dans la fonction recev de tcp mais ici direct
		if (read_socket_tcp(s, buffer_read, sizeof(buffer_read)) == -1) {
			fprintf(stderr, "[Erreur] read_socket_tcp\n");
		}
		printf("[Server:%d] réception : %s\n", pid, buffer_read);
		
		// printf("[Server:%d] émission : %s\n", pid, msg);
		// write
	}
	
	if (close_socket_tcp(s) == -1) {
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
