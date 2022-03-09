#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "../inc/socket_tcp.h"
#include "../inc/adresse_internet.h"

socket_tcp *init_socket_tcp(void) {
	socket_tcp *socket = (socket_tcp *) calloc(1, sizeof(socket_tcp));
	if (socket == NULL) {
		fprintf(stderr, "[Erreur] -> init_socket_tcp\n");
		return NULL;
	}
	socket->socket_fd = -1;
	return socket;
}

int connect_socket_tcp(socket_tcp *s, const char *adresse, uint16_t port) {
	if (s == NULL) {
		fprintf(stderr, "[Erreur] -> connect_socket_tcp : s vaut NULL\n");
		return -1;
	}
	adresse_internet *ad = adresse_internet_new(adresse, port);
	s->remote = ad;
	if ((s->socket_fd = socket(adresse_internet_get_domain(s->remote), SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "[Erreur] -> connect_socket_tcp : socket(%d, SOCK_STREAM, 0)\n", adresse_internet_get_domain(s->remote));
		return -1;
	}
	adresse_internet_free(s->remote);
	ad = adresse_internet_new(adresse, port);
	int ret = connect(s->socket_fd, (struct sockaddr *) &ad->sock_addr, sizeof(struct sockaddr_storage));
	if (ret != 0) {
		fprintf(stderr, "[Erreur] -> connect_socket_tcp : connect(%d)\n", s->socket_fd);
		return -1;
	}
	s->is_connected = 1;
	return 0;
}
	
int ajoute_ecoute_socket_tcp(socket_tcp *s, const char *adresse, uint16_t port) {
	if (s == NULL) {
		fprintf(stderr, "[Erreur] -> ajoute_ecoute_socket_tcp : s vaut NULL\n");
		return -1;
	}
	adresse_internet *ad = (adresse == NULL) ? adresse_internet_any(port) : adresse_internet_new(adresse, port);
	s->local = ad;
	int ret = bind(s->socket_fd, (struct sockaddr *) &ad->sock_addr, sizeof(struct sockaddr_storage));
	if (ret != 0) {
		fprintf(stderr, "[Erreur] -> ajoute_ecoute_socket_tcp : bind(%d)\n", s->socket_fd);
		return -1;
	}
	s->is_bind = 1;
	if (listen(s->socket_fd, SIZE_QUEUE) == -1) {
		fprintf(stderr, "[Erreur] -> ajoute_ecoute_socket_tcp : listen(%d, BACKLOG=%d)\n", s->socket_fd, SIZE_QUEUE);
		return -1;
	}
	s->is_listening = 1;
	return 0;
}

int accept_socket_tcp(const socket_tcp *s, socket_tcp *service) {
	if (s == NULL || service == NULL) {
		const char *msg = (s == NULL) ? "socket" : "service";
		fprintf(stderr, "[Erreur] -> accept_socket_tcp : %s est NULL\n", msg);
		return -1;
	}
	socklen_t len = sizeof(struct sockaddr_storage);
	struct sockaddr_storage ss;
	int socket_fd = accept(s->socket_fd, (struct sockaddr *) &ss, &len);
	if (socket_fd == -1) {
		fprintf(stderr, "[Erreur] -> accept_socket_tcp : accept : %d\n", socket_fd);
		return -1;
	}
	service->socket_fd = socket_fd;
	service->remote = malloc(sizeof(adresse_internet));
	if (service->remote == NULL) {
		fprintf(stderr, "[Erreur] -> accept_socket_tcp : malloc service->remote : %lu\n", sizeof(adresse_internet));
		return -1;
	}
	if (sockaddr_to_adresse_internet((struct sockaddr *) &ss, service->remote) != 0) {
		return -1;
	}
	if (getsockname(socket_fd, (struct sockaddr *) &ss, &len) == -1) {
		fprintf(stderr, "[Erreur] -> accept_socket_tcp : getsockname(%d)\n", socket_fd);
		return -1;
	}
	if (len > sizeof(struct sockaddr_storage)) {
		fprintf(stderr, "[Erreur] -> accept_socket_tcp : getsockname : buffer too small\n");
		return -1;
	}
	// Même chose pour local pour getsockname
	service->local = malloc(sizeof(adresse_internet));
	if (sockaddr_to_adresse_internet((struct sockaddr *) &ss, service->local) != 0) {
		fprintf(stderr, "[Erreur] -> accept_socket_tcp : sockaddr_to_adresse_internet\n");
		return -1;
	}
	// getnameinfo avec local
	int errnum;
	if ((errnum = getnameinfo((struct sockaddr *) &service->local->sock_addr, sizeof(struct sockaddr_storage), 
			service->local->nom, sizeof(service->local->nom),
			service->local->service, sizeof(service->local->service), NI_NAMEREQD)) != 0) {
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(errnum));
		return -1;
	}
	
	// getnameinfo avec remote
	if ((errnum = getnameinfo((struct sockaddr *) &service->remote->sock_addr, sizeof(struct sockaddr_storage), 
			service->remote->nom, sizeof(service->remote->nom),
			service->remote->service, sizeof(service->remote->service), NI_NAMEREQD)) != 0) {
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(errnum));
		return -1;
	}
	printf("accept_socket_tcp : service->local : %s:%s\n", service->local->nom, service->local->service);
	printf("accept_socket_tcp : service->remote : %s:%s\n", service->remote->nom, service->remote->service);
	return 0;
}

ssize_t write_socket_tcp(const socket_tcp *s, const void *buffer, size_t length) {
	if (s == NULL) {
		fprintf(stderr, "[Erreur] -> write_socket_tcp : s vaut NULL\n");
		return -1;
	}
	if (buffer == NULL) {
		fprintf(stderr, "[Erreur] -> write_socket_tcp : buffer vaut NULL\n");
		return -1;
	}
	return write(s->socket_fd, (void *) buffer, length);
}

ssize_t read_socket_tcp(const socket_tcp *s, const void *buffer, size_t length) {
	if (s == NULL) {
		fprintf(stderr, "[Erreur] -> read_socket_tcp : s vaut NULL\n");
		return -1;
	}
	if (buffer == NULL) {
		fprintf(stderr, "[Erreur] -> read_socket_tcp : buffer vaut NULL\n");
		return -1;
	}
	// printf("read_socket_tcp : s->socket_fd : %d - length : %lu\n", s->socket_fd, length);
	return read(s->socket_fd, (void *) buffer, length);
}

int close_socket_tcp(socket_tcp *s) {
	if (s == NULL) {
		fprintf(stderr, "[Erreur] -> close_socket_tcp : s vaut NULL\n");
		return -1;
	}
	adresse_internet_free(s->local);
	adresse_internet_free(s->remote);
	if (close(s->socket_fd) == -1) {
		fprintf(stderr, "[Erreur] -> close_socket_tcp : close : %d\n", s->socket_fd);
		// return -1;
	}
	s->socket_fd = -1;
	s->is_bind = 0;
	s->is_listening = 0;
	s->is_connected = 0;
	s->local = NULL;
	s->remote = NULL;
	free(s);
	s = NULL;
	return 0;
}

