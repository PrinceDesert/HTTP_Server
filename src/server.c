#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
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
 
// A REVOIR
// GMTLOCALTIME DANS LAST MODIFIED
*/

#define DEFAULT_INDEX_FILE_NAME "index.html"

// Ressources de connexion
void thread_allocation(socket_tcp *service); // Allocation d'un thread
void * run_connection_processing(void *arg); // Traitement de la connexion avec le client
void perror_r(int errnum, const char* s); // perror reetrant pour thread

// Traitement de la requête et l'envoie de la réponse
void process_request_and_response(socket_tcp *pservice, char *buffer_request);
int check_request_method(const char *request_method);
int check_request_url(char *request_url, size_t size_request_url, struct stat *statbuf, status_t *status);
int check_request_http_version_protocol(char *http_version_protocol);
char *get_mime_type_extension(const char *extension);
char *get_resquest_header_value(header_t headers[],  size_t size_headers, request_names_t request_name);
int parse_headers(char *buffer_request, char *line, size_t size_line, http_request *request, size_t index_request_header, http_response *response);
int read_and_write_file_url(char *url);
int set_header(header_t *h, char *name, char *value);

// Gestion des signaux de terminaisons
void connect_signals(void);
void handler(int signum);

pid_t pid;
pthread_mutex_t mutex;
socket_tcp *s;
socket_tcp *service;

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
	// printf("Acquisition du verrou\n");
	
	socket_tcp service = *(socket_tcp *) arg;
	
	struct timeval timeout = {
		.tv_sec = TIMEOUT_SOCKET,
		.tv_usec = 0,
	};
	if (setsockopt(service.socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		fprintf(stderr, "[Erreur] setsockopt SO_RCVTIMEO\n");
		return NULL;
	}
	if (setsockopt(service.socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1) {
		fprintf(stderr, "[Erreur] setsockopt SO_SNDTIMEO\n");
		return NULL;
	}
	
	char buffer_read[PIPE_BUF];
	buffer_read[PIPE_BUF - 1] = '\0';
	
	ssize_t n;
	if ((n = read_socket_tcp(&service, buffer_read, sizeof(buffer_read))) == -1) {
		fprintf(stderr, "[Erreur] read_socket_tcp %ld\n", n);
	} else {
		buffer_read[strlen(buffer_read)] = '\0';
		fprintf(stdout, "[Server:%d] réception : \n%s\n", pid, buffer_read);
		process_request_and_response(&service, buffer_read);
	}
	
	// Ferme juste le descripteur, car service encore utilisé pour les nouvelles connexions
	if (close(service.socket_fd) == -1) {
		fprintf(stderr, "[Erreur] close %d\n", service.socket_fd);
	}
	
	// Libération du verrou
	pthread_mutex_unlock(&mutex);
	// printf("Libération du verrou\n");
	pthread_exit(NULL); // return NULL;
}

void perror_r(int errnum, const char* s) {
	fprintf(stderr, "[Erreur] %s: %s\n", s, strerror(errnum));
}

void process_request_and_response(socket_tcp *pservice, char *buffer_request) {
	if (buffer_request == NULL) {
		fprintf(stderr, "[Erreur] process_request_and_response : buffer_request = NULL\n");
	}
	
	// Requête
	http_request request;
	size_t index_request_header = 0;
	
	// Réponse
	http_response response;
	response.status_code = OK;
	size_t index_response_header = 0;
	int length_response = 0;
	struct stat st;
	
	char line[1024];
	if (sscanf(buffer_request, "%[^\n]", line) == EOF) {
		fprintf(stderr, "[Erreur] process_request_and_response : sscanf line\n");
		return;
	}
	// Traitement de la ligne de commande
	if (sscanf(line, "%s%s%s", request.method, request.url, request.http_version_protocol) == EOF) {
		return;
	}
	if (check_request_method(request.method) == -1) {
		response.status_code = NOT_IMPLEMENTED;
	}
	status_t tmp_status = OK;
	if (check_request_url(request.url, sizeof(request.url), &st, &tmp_status) == -1) {
		response.status_code = tmp_status != OK ? tmp_status : NOT_FOUND;
	}
	if (check_request_http_version_protocol(request.http_version_protocol) == -1) {
		response.status_code = HTTP_VERSION_NOT_SUPPORTED;
	} else {
		if (snprintf(response.http_version_protocol, sizeof(response.http_version_protocol), "%s", request.http_version_protocol) == -1) {
			response.status_code = HTTP_VERSION_NOT_SUPPORTED;
		}
	}
		
	// Traitement des en-têtes (headers)
	if (response.status_code == OK || response.status_code == NOT_MODIFIED) {
		if (parse_headers(buffer_request, line, sizeof(line), &request, index_request_header, &response) == -1) {
			response.status_code = BAD_REQUEST;
		}
	}
	
	/**
	 * If Last Modified Since et mettre dans la réponse Last Modified
	*/
	char *val_header_if_modified_since = get_resquest_header_value(request.headers, sizeof(request.headers), IF_MODIFIED_SINCE);
	/*char *s_gmt = NULL;
	if (val_header != NULL && (s_gmt = strstr(val_header, "GMT")) != NULL) {
		// Enlève le GMT de la date de If Last Modified
		ssize_t index_of = s_gmt - val_header;
		for (size_t i = (size_t) index_of; i < sizeof(char) * strlen(val_header); i++) {
			val_header[i] = '\0';
		}
	}*/
	if (strlen(val_header_if_modified_since) > 0) {
		double seconds = 0.0;
		struct tm tm_if_modified_since;
		time_t t_if_modified_since;
		if (strptime(val_header_if_modified_since, "%a %b %d %H:%M:%S %Y %Z", &tm_if_modified_since) != NULL) {
			t_if_modified_since = mktime(&tm_if_modified_since);
			seconds = difftime(st.st_mtime, t_if_modified_since);
			if (seconds > 0) {
				response.status_code = NOT_MODIFIED;
			}
		}
	}
	
	// Check du corps de la request : pas implémenté
	/*
	fprintf(stdout, "Debut de l'analyse du corps de la requête\n");
	char data[PIPE_BUF];
	while (sscanf(buffer_request + n, "%[^\n]", line) != EOF) {
		// mettre dans le buffer
		sscanf(line, "%s", data);
		n += sizeof(char) * (strlen(line) + 1);
		printf("data : %s\n", data);
	}
	*/
	
	// Création des headers de réponse (en têtes)
	char buffer_headers_response[MAX_SIZE_HEADERS];
	// Date
	char buf_time[256];
	if (get_gmt_time(buf_time, NULL, sizeof(buf_time)) == -1) {
		fprintf(stderr, "[Erreur] -> create_request : get_gmt_time\n");
		sprintf(buf_time, "%s", "ERROR_DATE");
	}
	header_t h_date;
	if (set_header(&h_date, (char *) response_names[DATE_RESPONSE], buf_time) == 0) {
		response.headers[index_response_header++] = h_date;
	}
	// Server
	header_t h_server; 
	if (set_header(&h_server, (char *) response_names[SERVER], pservice->remote->nom) == 0) {
		response.headers[index_response_header++] = h_server;
	}
	// Content-Type
	const char *extension = get_filename_ext(request.url);
	const char *mime_type = get_mime_type_extension(extension); 
	if (mime_type == NULL) {
		response.status_code = NOT_FOUND;
		mime_type = (const char *) mime_names[PLAIN].type;
	}
	header_t h_content_type;
	if (set_header(&h_content_type, (char *) response_names[CONTENT_TYPE], (char *) mime_type) == 0) {
		response.headers[index_response_header++] = h_content_type;
	}
	// Content-Length
	header_t h_content_length;
	char number[1024];
	if (snprintf(number, sizeof(number), "%ld", st.st_size) != -1) {
		if (set_header(&h_content_length, (char *) response_names[CONTENT_LENGTH], number) == 0) {
			response.headers[index_response_header++] = h_content_length;
		}
	} else {
		fprintf(stderr, "[Erreur] process_request_and_response : stat number\n");
	}
	
	// Last-Modified
	header_t h_last_modified;
	memset(buf_time, '\0', sizeof(buf_time));
	if (get_gmt_time(buf_time, &st.st_mtime, sizeof(buf_time)) == -1) {
		fprintf(stderr, "[Erreur] -> create_request : get_local_time\n");
	} else {
		if (set_header(&h_last_modified, (char *) response_names[LAST_MODIFIED], buf_time) == 0) {
			response.headers[index_response_header++] = h_last_modified;
		}
	}
	
	// Concaténation des headers
	length_response += snprintf(buffer_headers_response + length_response, sizeof(buffer_headers_response), "%s %s\n", response.http_version_protocol, status_names[response.status_code]);
	fprintf(stdout, "Affichage des en-têtes de réponse\n");
	fprintf(stdout, "%s", buffer_headers_response);
	for (size_t i = 0; i < index_response_header; i++) {
		fprintf(stdout, "%s: %s\n", response.headers[i].name, response.headers[i].value);
		if (strlen(response.headers[i].name) > 0 && strlen(response.headers[i].value) > 0) {
			length_response += snprintf(buffer_headers_response + length_response, sizeof(buffer_headers_response), "%s: %s\n", response.headers[i].name, response.headers[i].value);
		}
	}
	// Ligne de séparation
	length_response += snprintf(buffer_headers_response + length_response, sizeof(buffer_headers_response), "\n");
	// Envoie du statut et des en-têtes
	write_socket_tcp(service, buffer_headers_response, sizeof(char) * (strlen(buffer_headers_response)));
	// Lecture et envoie du fichier
	if (response.status_code == OK) {
		if (read_and_write_file_url(request.url) == -1) {
			fprintf(stderr, "[Erreur] -> process_request_and_response : read_file_url %s\n", request.url);
		}
	}
}
	
/**
 * Vérifie que la méthode de la requête client est implémenté
*/
int check_request_method(const char *request_method) {
	if (request_method == NULL) {
		fprintf(stderr, "[Erreur] check_request_method : request_method vaut NULL\n");
		return -1;
	}
	size_t method_names_size = sizeof(method_names) / sizeof(method_names[0]);
	// Vérification de la méthode
	for (size_t i = 0; i < method_names_size; i++) {
		if (strncmp(method_names[i], request_method, sizeof(char) * strlen(request_method)) == 0) {
			return 0;
		}
	}
	fprintf(stderr, "[Erreur] process_request_and_response : méthode %s pas implémenté\n", request_method);
	return -1;
}
	
/**
 * Vérifie que l'url est syntaxiquement valide, et vérifie que le fichier est lisible
*/
int check_request_url(char *request_url, size_t size_request_url, struct stat *statbuf, status_t *status) {
	if (request_url == NULL) {
		fprintf(stderr, "[Erreur] check_request_url : request_url vaut NULL\n");
		return -1;
	}
	// Rajoute le fichier par défaut, si l'url contient que '/' -> "/index.html"
	if (strncmp(request_url, "/", sizeof(char) * strlen(request_url)) == 0) {
		if (snprintf(request_url, size_request_url, "%s%s", request_url, DEFAULT_INDEX_FILE_NAME) == -1) {
			fprintf(stderr, "[Erreur] check_request_url : snprintf adding slash\n");
			*status = NOT_FOUND;
			return -1;
		}
	}
	// Rajoute un point pour le chemin -> "./index.html"
	if (request_url[0] == '/') {
		char *copy = strndup(request_url, sizeof(char) * (strlen(request_url) + 1));
		if (snprintf(request_url, size_request_url, "%c%s", '.', copy) == -1) {
			fprintf(stderr, "[Erreur] check_request_url : snprintf adding dot\n");
			*status = NOT_FOUND;
			return -1;
		}
		free(copy);
	}
	// Récupère information du fichier
	if (stat(request_url, statbuf) == -1) {
		fprintf(stderr, "[Erreur] check_request_url : stat %s\n", request_url);
		*status = NOT_FOUND;
		return -1;
	}
	// Vérifie qu'il est accessible
	int fd = open(request_url, O_RDONLY, statbuf->st_mode);
	if (fd == -1) {
		*status = FORBIDDEN;
		return -1;
	}
	close(fd);
	return 0;
}
	
/**
 * Vérifie la version du protocole HTTP
*/
int check_request_http_version_protocol(char *http_version_protocol) {
	if (strncmp(http_version_protocol, HTTP_VERSION_PROTOCOL, sizeof(char) * strlen(HTTP_VERSION_PROTOCOL)) != 0) {
		fprintf(stderr, "[Erreur] process_request_and_response : http version du protocole %s incorrect, la version doit être %s\n", http_version_protocol, HTTP_VERSION_PROTOCOL);
		return -1;
	}
	return 0;
}	
	
/**
 * Récupère le type mime associé à l'extension
*/
char *get_mime_type_extension(const char *extension) {
	if (extension == NULL) {
		fprintf(stderr, "[Erreur] -> get_mime_type_extension : extension vaut NULL\n");
		return NULL;
	}
	size_t size_mime_names = sizeof(mime_names) / sizeof(mime_names[0]);
	for (size_t i = 0; i < size_mime_names; i++) {
		size_t size_mime_names_extension =  sizeof(mime_names[i].extension) / sizeof(mime_names[i].extension[0]);
		// printf("size_mime_names_extension %lu - i : %s - ext : %s\n", size_mime_names_extension, mime_names[i].type, mime_names[i].extension[0]);
		for (size_t j = 0; j < size_mime_names_extension; j++) {
			if (strncmp(extension, mime_names[i].extension[j], sizeof(char) * strlen(extension)) == 0) {
				return (char *) mime_names[i].type;
			}
		}
	}
	return NULL;
}
	
/**
 * Récupère la valeur de l'en tête à associé au nom du header
*/
char *get_resquest_header_value(header_t headers[], size_t size_headers, request_names_t request_name) {
	for (size_t i = 0; i < size_headers; i++) {
		if (strncmp(headers[i].name, request_names[request_name],
			sizeof(char) * (strlen(headers[i].name))) == 0) {
			return headers[i].value;
		}
	}
	return NULL;
}
	
/**
 * Analyse les en têtes et les enregistrent dans http_request
*/
int parse_headers(char *buffer_request, char *line, size_t size_line, http_request *request, size_t index_request_header, http_response *response) {
	size_t n = sizeof(char) * (strlen(line) + 1);
	size_t size_request_headers = sizeof(request_names) / sizeof(request_names[0]);
	int errnum; 
	while ((errnum = sscanf(buffer_request + n, "%[^\n]", line)) != EOF && index_request_header < MAX_NUMBER_HEADERS) {
		// printf("line : %s\n", line);
		if (strchr(line, ':') != NULL) {
			if (sscanf(line, "%s%[0-9a-zA-Z $&+,:;=?@#|'\"<>.^*()%%!-]", request->headers[index_request_header].name, request->headers[index_request_header].value) == EOF) {
				response->status_code = BAD_REQUEST;
				return -1;
			}
			trim(request->headers[index_request_header].name);
			trim(request->headers[index_request_header].value);
			// Enlève le caractère ':'
			request->headers[index_request_header].name[strlen(request->headers[index_request_header].name) - 1] = '\0';
			trim(request->headers[index_request_header].name);
			trim(request->headers[index_request_header].value);
			// Vérifie qu'il existe dans les headers traités
			for (size_t i = 0; i < size_request_headers; i++) {
				if (strncmp(request->headers[index_request_header].name, request_names[i],
					sizeof(char) * (strlen(request->headers[index_request_header].name))) == 0) {
					index_request_header++;
					break;
				}
			}
			fprintf(stdout, "%s%s\n", request->headers[index_request_header].name, request->headers[index_request_header].value);
			n += sizeof(char) * (strlen(line) + 1);
		} else if (strncmp(line, EMPTY_LINE, sizeof(char) * strlen(line)) == 0) {
			// Ligne de séparation
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
			memset(line, '\0', size_line);
			if (is_separator_line) {
				n = n_tmp;
				break;
			}
		} else {
			// Erreur dans le format
			response->status_code = BAD_REQUEST;
			return -1;
		}
	}
	if (errnum == EOF) {
		response->status_code = BAD_REQUEST;
		return -1;
	}
	return 0;
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
	if (n_read == -1) {
		fprintf(stderr, "[Erreur] -> read_file_url : read : %s\n", strerror(errno));
		return -1;
	}
	return 0;
}	

int set_header(header_t *h, char *name, char *value) {
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
		*h->name = '\0';
		*h->value = '\0';
		return -1;
	}
	if (snprintf(h->value, sizeof(h->value), "%s", value) == -1) {
		fprintf(stderr, "[Erreur] -> set_header : snprintf %s(value)\n", value);
		*h->name = '\0';
		*h->value = '\0';
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
