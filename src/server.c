#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "../inc/adresse_internet.h"
#include "../inc/socket_tcp.h"
#include "../inc/config.h"
#include "../inc/http_response_def.h"

/**
 * Fichier server.c
 * Todo : timeout client et server en émission et réception
 * EN TCP : mettre un select sur le descripteur de fichier voir cours
 * utilisé setsockopt ?
 * // mettre un timeout  select pour accept(délai d'attente max d'un client) && receive mais dans la fonction recev de tcp mais ici direct
 * pour le fstat since date : récupèrer la date du fstat et comparé avec la date du since de la request
 * comparé les protocols http/1.1 si c le meme meme si ça sert pas
 * gérer les signaux
 * commenté fichiers sources commentés de vos bibliothèques et de votre serveur http, client
 * des tests de vos bibliothèques et de votre serveur,
 * un rapport et manuel d'utilisation
 * ressources : 
 * https://code.tutsplus.com/tutorials/http-headers-for-dummies--net-8039
 * https://github.com/AaronKalair/C-Web-Server
 * List des champs http : https://en.wikipedia.org/wiki/List_of_HTTP_header_fields
*/


typedef struct {
	const char *protocol; // HTTP/1.x x = version
	const status_t status_code; // 200 OK, 206 Partial Content, 404 Not Found, 401 Unauthorized, 403 Forbidden
	header_t headers[MAX_SIZE_HEADERS_FIELDS]; // entêtes de la réponse, avec au max 64 champs
} _http_response;
typedef  _http_response http_response;

// headers["host"] = localhost -> s->getname()

// Ressources de connexion
void thread_allocation(socket_tcp *service); // Allocation d'un thread
void * run_connection_processing(void *arg); // Traitement de la connexion avec le client
void perror_r(int errno, const char* s); // perror reetrant pour thread

// Traitement de la requête et l'envoie de la réponse
int parse_request(char *buffer_request);
int create_response(void);


pid_t pid;
pthread_mutex_t mutex;
socket_tcp *s;


// #define VERSION_HTTP "HTTP/1.1" HTTP/1.1 = Version 1.1 = version à mettre dans le define

int main(void) {
	pid = getpid();
	mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
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
	printf("[Serveur:%d] En écoute sur %s:%d\n", pid, ADDRESS, PORT);
	if (ajoute_ecoute_socket_tcp(s, ADDRESS, PORT) == -1) {
		return EXIT_FAILURE;
	}
	
	socket_tcp *service = init_socket_tcp();
	
	int errnum;
	printf("[Serveur:%d] En attente de client(s) (MAX_CLIENT_QUEUE=%d)\n", pid, SIZE_QUEUE);
	while ((errnum = accept_socket_tcp(s, service)) != -1) {
		thread_allocation(service);
	}
	if (errnum == -1) {
		fprintf(stderr, "[Erreur] accept_socket_tcp\n");
		return EXIT_FAILURE;
	}
	
	if (close_socket_tcp(service) == -1) {
		fprintf(stderr, "[Erreur] close_socket_tcp %d\n", service->socket_fd);
	}
	
	if (close_socket_tcp(s) == -1) {
		
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
	if (arg == NULL) return NULL;
	
	// Acquisition du verrou
	pthread_mutex_lock(&mutex);
	
	socket_tcp service = *(socket_tcp *) arg;
	
	char buffer_read[BUFFER_SIZE];
	buffer_read[BUFFER_SIZE - 1] = '\0';
	
	ssize_t n;
	if ((n = read_socket_tcp(&service, buffer_read, sizeof(buffer_read))) == -1) {
		fprintf(stderr, "[Erreur] read_socket_tcp %ld\n", n);
		return NULL;
	}
	buffer_read[strlen(buffer_read)] = '\0';
	fprintf(stdout, "[Server:%d] réception : \n%s\n", pid, buffer_read);
	
	if (!parse_request(buffer_read)) {
		fprintf(stderr, "[Erreur] parse_request\n");
		return NULL;
	}
	
	if ((n = write_socket_tcp(&service, buffer_read, sizeof(char) * (strlen(buffer_read) + 1))) == -1) {
		fprintf(stderr, "[Erreur] write_socket_tcp %ld\n", n);
		return NULL;
	}
	
	
	// pas de close sur la socket, elle est libéré dans le main car c'est le même pointeur service qui est utilisé et donné à chaque thread
	
	// Libération du verrou
	pthread_mutex_unlock(&mutex);
	
	pthread_exit(NULL); // return NULL;
}

void perror_r(int errno, const char* s) {
	fprintf(stderr, "[Erreur] %s: %s\n", s, strerror(errno));
}

int parse_request(char *buffer_request) {
	// http_response
	// GET / HTTP/1.0 -> parse et affecte
	 
	// Récupère la command qui est la première ligne
	// strtok = '\n'
	// sinon erreur fomat command
	
	// récupère l'image et on lui envoie et le client lit les données du données
	 
	 // le serveur écrit la page html en dessous du header
	 
	char method[40];
	char url[40];
	char version[40];
	 
	char line[128];
	if (sscanf(buffer_request, "%[^\n]", line) == EOF) {
		return 0; 
	}
	 
	if (sscanf(buffer_request, "%s %s %s", method, url, version) == EOF) {
		return 0; 
	}
	/*
	if (strncmp(method, GET, strlen(method) == 0) {
		
	} */
	
	fprintf(stdout, "command : method=%s, url=%s, version=%s\n", method, url, version);
	
	
	// Pour les fichiers binaires images, texte
	// Requête : GET ./paysage.png
	/*
	 * Réponse :
	 * HTTP/1.1 200 OK
	 * Content-Type : image/img
	 * \n
	 * 
	 * write(réponse)
	 * while(n = read(fd, buf, strlen(buf) + 1))
	 *    write to socket(service, buf, n)
	*/
	 
	return 1;
}

int create_response() {
	return 0;
}

