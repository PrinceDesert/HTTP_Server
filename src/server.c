#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "../inc/adresse_internet.h"
#include "../inc/socket_tcp.h"
#include "../inc/config.h"

/**
 * Fichier server.c
 * Todo : timeout client et server en émission et réception
 * EN TCP : mettre un select sur le descripteur de fichier voir cours
 * utilisé setsockopt ?
 * gérer les signaux
 * commenté fichiers sources commentés de vos bibliothèques et de votre serveur http, client
 * des tests de vos bibliothèques et de votre serveur,
 * un rapport et manuel d'utilisation
 * ressources : 
  * https://code.tutsplus.com/tutorials/http-headers-for-dummies--net-8039
  * https://github.com/AaronKalair/C-Web-Server
*/

void thread_allocation(socket_tcp *service); // Allocation d'un thread
void * run_connection_processing(void *arg); // Traitement de la connexion avec le client

void perror_r(int errno, const char* s); // perror reetrant pour thread

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
		fprintf(stderr, "[Erreur] accept_socket_tcp\n");
		return EXIT_FAILURE;
	}
	
	if (close_socket_tcp(s) == -1) {
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

void thread_allocation(socket_tcp *service) {
	if (service == NULL) return;
	fprintf(stdout, "[Serveur:%d] Allocation d'une ressource (thread)\n", pid);
	int return_value;
	pthread_attr_t attr;
	// Initiliase les attributs du thread
	if ((return_value = pthread_attr_init(&attr)) != 0) {
		perror_r(return_value, "pthread_attr_init");
		exit(EXIT_FAILURE);
	}
	// Thread en mode détaché
	if ((return_value = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
		perror_r(return_value, "pthread_attr_setdetachstate");
		exit(EXIT_FAILURE);
	}
	pthread_t th;
	// Création du thread avec sa start_routine service
	if ((return_value = pthread_create(&th, &attr, run_connection_processing, (void *) service)) != 0) {
		perror_r(return_value, "pthead_create");
		exit(EXIT_FAILURE);
	}
	// Destruction des attributs initialisés
	if ((return_value = pthread_attr_destroy(&attr)) != 0) {
		perror_r(return_value, "pthread_attr_destroy");
		exit(EXIT_FAILURE);
	}
}

void * run_connection_processing(void *arg) {
	
	// return NULL;
	pthread_exit(NULL);
}

void perror_r(int errno, const char* s) {
	fprintf(stderr, "[Erreur] %s: %s\n", s, strerror(errno));
}
