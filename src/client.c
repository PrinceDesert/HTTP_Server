#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "../inc/adresse_internet.h"
#include "../inc/socket_tcp.h"
#include "../inc/config.h"
#include "../inc/http_request_def.h"
#include "../inc/utils.h"

/**
	typedef struct request {
		header_t // entete
			-> struct avec method, date, useragent ect 
		
	}
*/

typedef struct {
	const methods_t method; // GET, POST
	const char *path; // URL du fichier
	const char *protocol; // HTTP/1.x x = version
	header_t headers[MAX_SIZE_HEADERS_FIELDS]; // entêtes de la requête, avec au max 64 champs
	const char *body; // corps de la requête
} _http_request;
typedef  _http_request http_request;


// Traitement et envoie de la requête
int create_request(void);


http_request request;


int main(void) {
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
		close_socket_tcp(s);
		return EXIT_FAILURE;
	}
	
	
	// char buffer_send[1024];
	create_request();
	
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

// mettre les paramètres genre method = GET que on met direct dans la struct
int create_request() {
	
	char buf_time[256];
	if (get_gmt_time(buf_time, sizeof(buf_time)) == -1) {
		fprintf(stderr, "[Erreur] -> create_request : get_gmt_time\n");
		return 0;
	};
	
	header_t h_date; // = {s, buf_time};
	if (snprintf(h_date.name, sizeof(h_date.name), "%s", request_names[DATE]) == -1) {
		fprintf(stderr, "[Erreur] -> create_request : snprintf %s\n", request_names[DATE]);
		return 0;
	};
	if (snprintf(h_date.value, sizeof(h_date.value), "%s", buf_time) == -1) {
		fprintf(stderr, "[Erreur] -> create_request : snprintf %s\n", buf_time);
		return 0;
	};
	
	header_t h_accept;
		if (snprintf(h_accept.name, sizeof(h_accept.name), "%s", request_names[ACCEPT]) == -1) {
		fprintf(stderr, "[Erreur] -> create_request : snprintf %s\n", request_names[ACCEPT]);
		return 0;
	};
	if (snprintf(h_accept.value, sizeof(h_accept.value), "%s", mime_names[ACCEPT].type) == -1) {
		fprintf(stderr, "[Erreur] -> create_request : snprintf %s\n", buf_time);
		return 0;
	};
	
	http_request r = (http_request) {GET, "/", HTTP_PROTOCOL, {h_date, h_accept}, "<!DOCTYPE html><html lang=\"fr\"></html>"};
	
	printf("%s %s %s\n", method_names[r.method], r.path, r.protocol);
	size_t i = 0;
	while (i < MAX_SIZE_HEADERS_FIELDS && strlen(r.headers[i].name) != 0) {
		printf("%s : %s\n", r.headers[i].name, r.headers[i].value);
		i++;
	}
	printf("\n");
	printf("%s\n", r.body);
	
	return 1;
}
