#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <adresse_internet.h>
#include <socket_tcp.h>
#include <config.h>
#include <http_def.h>
#include <utils.h>

/* 
	* Utilisation de l'option "-I inc" pour inclure les fichiers d'en têtes sans spécifier le chemin à chaque include 
	#include "../inc/adresse_internet.h"
	#include "../inc/socket_tcp.h"
	#include "../inc/config.h"
	#include "../inc/http_def.h"
	#include "../inc/utils.h"
*/

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
 * // concat with sprintf : https://stackoverflow.com/questions/2674312/how-to-append-strings-using-sprintf
*/

#define DEFAULT_INDEX_FILE_NAME "index.html"


// headers["host"] = localhost -> s->getname()

// Ressources de connexion
void thread_allocation(socket_tcp *service); // Allocation d'un thread
void * run_connection_processing(void *arg); // Traitement de la connexion avec le client
void perror_r(int errnum, const char* s); // perror reetrant pour thread

// Traitement de la requête et l'envoie de la réponse
int process_request(socket_tcp *pservice, char *buffer_request);
int read_and_write_file_url(char *url);
int set_header(header_t *h, const char *name, const char *value);

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
	
	close_socket_tcp(service);
	
	if (close_socket_tcp(s) == -1) {
		fprintf(stderr, "[Erreur] close_socket_tcp server %d\n", s->socket_fd);
	}
	
	
	return EXIT_SUCCESS;
}

void thread_allocation(socket_tcp *pservice) {
	if (pservice == NULL) return;
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
	if ((return_value = pthread_create(&th, &attr, run_connection_processing, (void *) pservice)) != 0) {
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
	printf("Acquisition du verrou\n");
	
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
	
	if (!process_request(&service, buffer_read)) {
		fprintf(stderr, "[Erreur] process_request\n");
	}
	
	// Ferme juste le descripteur, car service encore utilisé pour les nouvelles connexions
	printf("Close socket \n");
	if (close(service.socket_fd) == -1) {
		fprintf(stderr, "[Erreur] close %d\n", service.socket_fd);
	}
	
	// Libération du verrou
	pthread_mutex_unlock(&mutex);
	printf("Libération du verrou\n");
	
	pthread_exit(NULL); // return NULL;
}

void perror_r(int errnum, const char* s) {
	fprintf(stderr, "[Erreur] %s: %s\n", s, strerror(errnum));
}

int process_request(socket_tcp *pservice, char *buffer_request) {
	if (buffer_request == NULL) {
		fprintf(stderr, "[Erreur] process_request : buffer_request = NULL\n");
	}
		
	// 1ère ligne
	http_request request;
	size_t index_request_header = 0;
		
	char line[128];
	if (sscanf(buffer_request, "%[^\n]", line) == EOF) {
		return 0;
	}
	// Récupère la ligne de commande
	if (sscanf(line, "%s %s %s", request.method, request.url, request.http_version_protocol) == EOF) {
		return 0; 
	}
	// METHOD	
	size_t method_names_size = sizeof(method_names) / sizeof(method_names[0]);
	int is_found = 0;
	// Vérification de la méthode
	for (size_t i = 0; i < method_names_size; i++) {
		if (strncmp(method_names[i], request.method, sizeof(char) * strlen(request.method)) == 0) {
			is_found = 1;
		}
	}
	if (!is_found) {
		fprintf(stderr, "[Erreur] process_request : méthode %s pas implémenté\n", request.method);
		// Envoyer un header erreur
		response.status_code = BAD_REQUEST;
	}
	// URL
	// Rajoute le fichier par défaut, si l'url contient que '/'
	if (strncmp(request.url, "/", sizeof(char) * strlen(request.url)) == 0) {
		char *copy = strndup(request.url, sizeof(request.url));
		snprintf(request.url, sizeof(request.url), "%s%s", request.url, DEFAULT_INDEX_FILE_NAME);
		free(copy);
	}
	// Rajoute un point pour le chemin
	if (request.url[0] == '/') {
		char *copy = strndup(request.url, sizeof(request.url));
		snprintf(request.url, sizeof(request.url), "%c%s", '.', copy);
		free(copy);
	}
	int fd = open(request.url, O_RDONLY);
	if (fd == -1) {
		response.status_code = NOT_FOUND;
	}
	close(fd);
	// VERSION
	// Vérification de la version
	if (strncmp(request.http_version_protocol, HTTP_VERSION_PROTOCOL, sizeof(char) * strlen(HTTP_VERSION_PROTOCOL)) != 0) {
		fprintf(stderr, "[Erreur] process_request : http version du protocole %s incorrect, la version doit être %s\n", request.http_version_protocol, HTTP_VERSION_PROTOCOL);
		// Envoyer un header erreur
		response.status_code = HTTP_VERSION_NOT_SUPPORTED;
	} else {
		snprintf(response.http_version_protocol, sizeof(response.http_version_protocol), "%s", HTTP_VERSION_PROTOCOL);
	}
	
	size_t n = sizeof(char) * (strlen(line) + 1);
	printf("n : %lu\n", n);
	
	size_t size_request_headers = sizeof(request_names) / sizeof(request_names[0]);
	
	int errnum; 
	while ((errnum = sscanf(buffer_request + n, "%[^\n]", line)) != EOF && index_request_header < MAX_NUMBER_HEADERS) {
		// printf("line : %s\n", line);
		if (strchr(line, ':') != NULL) {
			if (sscanf(line, "%s%s", request.headers[index_request_header].name, request.headers[index_request_header].value) == EOF) {
				response.status_code = BAD_REQUEST;
				break;
			}
			// Enlève le caractère ':'
			request.headers[index_request_header].name[strlen(request.headers[index_request_header].name) - 1] = '\0';
			
			// Vérifie qu'il existe dans les headers traités
			for (size_t i = 0; i < size_request_headers; i++) {
				if (strncmp(request.headers[index_request_header].name, request_names[i], 
					sizeof(char) * (strlen(request.headers[index_request_header].name))) == 0) {
					index_request_header++;
					break;
					printf("header match : %s\n", request.headers[index_request_header].name);
				}
				printf("header analyse : %s\n", request.headers[index_request_header].name);
			}
			n += sizeof(char) * (strlen(line) + 1);
			// printf("header -> name : %s - value : %s\n", request.headers[index_request_header].name, request.headers[index_request_header].name);
		} else if (strncmp(line, EMPTY_LINE, sizeof(char) * strlen(line)) == 0) {
			// Ligne de séparation
			printf("ligne de séparation\n");
			// Vérifie les lignes suivantes
			size_t n_tmp = n + sizeof(char) * (strlen(line) + 1);
			n += sizeof(char) * (strlen(line) + 1);
			int is_separator_line = 1;
			while (sscanf(buffer_request + n_tmp, "%[^\n]", line) != EOF) {
				n_tmp += sizeof(char) * (strlen(line) + 1);
				if (strchr(line, ':') != NULL) {
					is_separator_line = 0;
					break;
				}
			}
			memset(line, '\0', sizeof(line));
			if (is_separator_line) {
				n = n_tmp;
				break;
			}
		} else {
			// Erreur dans le format
			response.status_code = BAD_REQUEST;
			break;
		}
	}
	if (errnum == EOF) {
		response.status_code = BAD_REQUEST;
	}
	
	
	// Vérification affichage des headers
	for (size_t i = 0; i < index_request_header; i++) {
		printf("[VERIF] %s: %s\n", request.headers[i].name, request.headers[i].value);
	}
	
	// Check du corps de la request
	printf("Debut de l'analyse du corps de la requête\n");
	char data[PIPE_BUF];
	while (sscanf(buffer_request + n, "%[^\n]", line) != EOF) {
		// mettre dans le buffer
		sscanf(line, "%s", data);
		n += sizeof(char) * (strlen(line) + 1);
		printf("data : %s\n", data);
	}
	
	
	const char *extension = get_filename_ext(request.url);
	printf("Extension : %s\n", extension);
	
	// Création des headers de répoonse (en têtes)
	
	char buffer_response[900000];
	// à remplir en fonction des valeurs
	http_response response;
	size_t index_response_header = 0;
	
	// Date
	char buf_time[256];
	if (get_gmt_time(buf_time, sizeof(buf_time)) == -1) {
		fprintf(stderr, "[Erreur] -> create_request : get_gmt_time\n");
		return 0;
	};
	header_t h_date;
	if (set_header(&h_date, response_names[DATE_RESPONSE], buf_time) == 0) {
		response.headers[index_response_header++] = h_date;
	}
	printf("%s : %s\n", h_date.name, h_date.value);
	// Server
	header_t h_server; 
	if (set_header(&h_server, response_names[SERVER], pservice->remote->nom) == 0) {
		response.headers[index_response_header++] = h_server;
	}
	// Content-Type
	header_t h_content_type;
	if (set_header(&h_content_type, response_names[CONTENT_TYPE], "text/html") == 0) {
		// faire un truc ici genre renvoyé une 401
		response.headers[index_response_header++] = h_content_type;
	}
	// Content-Length
	header_t h_content_length;
	struct stat st;
	if (stat(request.url, &st) != -1) {
		char number[1024];
		if (snprintf(number, sizeof(number), "%ld", st.st_size) != -1) {
			if (set_header(&h_content_type, response_names[CONTENT_LENGTH], number) == 0) {
				response.headers[index_response_header++] = h_content_length;
			}
		} else {
			fprintf(stderr, "[Erreur] process_request : stat number\n");
		}
	} else {
		fprintf(stderr, "[Erreur] process_request : stat %s\n", request.url);
	}
	
	// Concaténation des headers
	length_response += snprintf(buffer_response + length_response, sizeof(buffer_response), "%s %s\n", response.http_version_protocol, status_names[response.status_code]);
	
	length_response += snprintf(buffer_response + length_response, sizeof(buffer_response), "%s: %s\n", h_date.name, h_date.value);
	// faire un for pour chaque header avec index_response
	
	length_response += snprintf(buffer_response + length_response, sizeof(buffer_response), "Content-Type: text/html\n");
	length_response += snprintf(buffer_response + length_response, sizeof(buffer_response), "\n"); // Ligne de séparation*/
	// Envoie du header
	write_socket_tcp(service, buffer_response, sizeof(char) * (strlen(buffer_response)));
	

	if (read_and_write_file_url(request.url) == -1) {
		fprintf(stderr, "[Erreur] -> process_request : read_file_url %s\n", request.url);
	}
	
	
	return 1;
}
	
int read_and_write_file_url(char *url) {
	if (url == NULL) {
		fprintf(stderr, "[Erreur] -> read_file_url : url vaut NULL\n");
		return -1;
	}
	int fd = open(url, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return -1;
	}
	char buf_read[PIPE_BUF];
	size_t count_read = sizeof(buf_read);
	ssize_t n_read;
	while ((n_read = read(fd, &buf_read, count_read)) > 0) {
		if (write_socket_tcp(service, &buf_read, (size_t) n_read) == -1) {
			fprintf(stderr, "[Erreur] -> read_file_url : write_socket_tcp\n");
			return -1;
		}
	}
	
	/*char *idx = NULL;
	int length = 0;
	char tmp[size_buffer + 127];
	while ((n_read = read(fd, &buf_read, count_read)) > 0) {
		buf_read[n_read] = '\0';
		if (strncpy(buffer, (char *) buf_read, (size_t) n_read) == NULL) {
			fprintf(stderr, "[Erreur] -> read_file_url : strncpy : %s\n", strerror(errno));
			return -1;
		}
		idx = buf_read;
		while (*idx) {
			length += sprintf(tmp + length, "%s", idx);
			strncat(buffer+length, (char *) tmp, (long unsigned int)length);
			idx += strlen(idx) + 1;
		}
	}*/
	
	if (n_read == -1) {
		fprintf(stderr, "[Erreur] -> read_file_url : read : %s\n", strerror(errno));
		return -1;
	}
	return 0;
}
	
int set_header(header_t *h, const char *name, const char *value) {
	if (name == NULL) {
		fprintf(stderr, "[Erreur] -> set_header : name vaut NULL\n");
		return -1;
	}
	if (value == NULL) {
		fprintf(stderr, "[Erreur] -> set_header : name vaut NULL\n");
		return -1;
	}
	if (snprintf(h->name, sizeof(h->name), "%s", name) == -1) {
		fprintf(stderr, "[Erreur] -> set_header : snprintf %s(name)\n", name);
		return -1;
	}
	if (snprintf(h->value, sizeof(h->value), "%s", value) == -1) {
		fprintf(stderr, "[Erreur] -> set_header : snprintf %s(value)\n", value);
		return -1;
	}
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
