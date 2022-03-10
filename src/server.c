#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "../inc/adresse_internet.h"
#include "../inc/socket_tcp.h"
#include "../inc/config.h"
#include "../inc/http_def.h"
#include "../inc/utils.h"

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

// Gestion des signaux de terminaisons
void connect_signals(void);
void handler(int signum);


pid_t pid;
pthread_mutex_t mutex;
socket_tcp *s;
socket_tcp *service;


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
	
	// On autorise l'adresse locale à être réutilisée si on ne l'utilise plus
	// Setting of SO_REUSEADDR should remove binding problems
	// https://stackoverflow.com/questions/10619952/how-to-completely-destroy-a-socket-connection-in-c
	int reuse = 1;
	if (setsockopt(s->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
		perror("setsockopt");
		return EXIT_FAILURE;
	}
	
	printf("[Serveur:%d] En écoute sur %s:%d\n", pid, ADDRESS, PORT);
	if (ajoute_ecoute_socket_tcp(s, ADDRESS, PORT) == -1) {
		return EXIT_FAILURE;
	}
	
	connect_signals();
	
	service = init_socket_tcp();
	
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
		fprintf(stderr, "[Erreur] close_socket_tcp service %d\n", service->socket_fd);
	}
	
	if (close_socket_tcp(s) == -1) {
		fprintf(stderr, "[Erreur] close_socket_tcp server %d\n", s->socket_fd);
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
	
	if ((n = write_socket_tcp(&service, "HELLO", sizeof(char) * (strlen("HELLO") + 1))) == -1) {
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
	if (buffer_request == NULL) {
		fprintf(stderr, "[Erreur] parse_request : buffer_request = NULL\n");
	}
		 
	char method[64];
	char url[64];
	char http_version_protocol[64];
	 
	char line[128];
	if (sscanf(buffer_request, "%[^\n]", line) == EOF) {
		return 0; 
	}
	// Récupère la ligne de commande
	if (sscanf(line, "%s %s %s", method, url, http_version_protocol) == EOF) {
		return 0; 
	}
	size_t method_names_size = sizeof(method_names) / sizeof(method_names[0]);
	int is_found = 0;
	// Vérification de la méthode
	for (size_t i = 0; i < method_names_size; i++) {
		if (strncmp(method_names[i], method, sizeof(char) * strlen(method)) == 0) {
			is_found = 1;
		}
	}
	if (!is_found) {
		fprintf(stderr, "[Erreur] parse_request : méthode %s pas implémenté\n", method);
		// Envoyer un header erreur
	}
	// Vérification de la version
	if (strncmp(http_version_protocol, HTTP_VERSION_PROTOCOL, sizeof(char) * strlen(HTTP_VERSION_PROTOCOL)) != 0) {
		fprintf(stderr, "[Erreur] parse_request : http version du protocole %s incorrect, la version doit être %s\n", http_version_protocol, HTTP_VERSION_PROTOCOL);
		// Envoyer un header erreur
	}
	
	size_t n = sizeof(char) * (strlen(line) + 1);
	printf("n : %lu\n", n);
	char nom[256];
	char value[256];
	while (sscanf(buffer_request + n, "%[^\n]", line) != EOF) {
		printf("line : %s\n", line);
		if (strchr(line, ':') != NULL) {
			sscanf(line, "%s%s", nom, value);
			n += sizeof(char) * (strlen(line) + 1);
			printf("header -> name : %s - value : %s\n", nom, value);
		} else if (strncmp(line, EMPTY_LINE, sizeof(char) * strlen(line)) == 0) {
			// Ligne de séparation
			printf("ligne de séparation\n");
			break;
		} else {
			// Erreur dans le format
			break;
		}
	}
	
	// Check du corps de la request
	char data[256];
	while (sscanf(buffer_request + n, "%[^\n]", line) != EOF) {
		// mettre dans le buffer
		sscanf(line, "%s", data);
		n += sizeof(char) * (strlen(line) + 1);
		printf("data : %s\n", data);
	}
	
	
	char buf_time[256];
	if (get_gmt_time(buf_time, sizeof(buf_time)) == -1) {
		fprintf(stderr, "[Erreur] -> create_request : get_gmt_time\n");
		return 0;
	};
	header_t h_date;
	if (snprintf(h_date.name, sizeof(h_date.name), "%s", request_names[DATE]) == -1) {
		fprintf(stderr, "[Erreur] -> parse_request : snprintf %s\n", request_names[DATE]);
		return 0;
	};
	if (snprintf(h_date.value, sizeof(h_date.value), "%s", buf_time) == -1) {
		fprintf(stderr, "[Erreur] -> parse_request : snprintf %s\n", buf_time);
		return 0;
	};
	
	/*
	if (strncmp(method, GET, strlen(method) == 0) {
		
	} */
	
	
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

void connect_signals() {
	sigset_t sigset;
	if (sigemptyset(&sigset) == -1) {
		perror("sigemptyset");
		exit(EXIT_FAILURE);
	}
	struct sigaction action;
	action.sa_handler = handler;
	action.sa_mask = sigset;
	action.sa_flags = 0;
	if (sigaction(SIGINT, &action, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGQUIT, &action, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGTERM, &action, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
	action.sa_handler = SIG_IGN;
	if (sigaction(SIGCHLD, &action, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
}

void handler(int signum) {
	if (signum != SIGINT && signum != SIGQUIT && signum != SIGTERM) {
		fprintf(stderr, "handler : unexpected signal number %d\n", signum);
		exit(EXIT_FAILURE);
	}
	if (close_socket_tcp(service) == -1) {
		fprintf(stderr, "[Erreur] close_socket_tcp service %d\n", service->socket_fd);
	}
	if (close_socket_tcp(s) == -1) {
		fprintf(stderr, "[Erreur] close_socket_tcp server %d\n", s->socket_fd);
	}
	fprintf(stdout, "[Serveur:%d] Signal de terminaison reçu\n", pid);
	exit(EXIT_SUCCESS);
}
