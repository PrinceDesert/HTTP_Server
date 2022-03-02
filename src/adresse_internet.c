#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include "adresse_internet.h"

#ifndef NI_MAXHOST
// NI_MAXHOST n'est pas définie pas SUSv4 mais peut l'être sur certains systèmes.
#define NI_MAXHOST 1025
#endif

#define DEFAULT_SA_FAMILY AF_INET
#define MAX_NUMBERS_PORT 8
#define BUFFER_SIZE 256


	
adresse_internet * _adresse_internet_new(sa_family_t sa_family, const int flag, const char *nom, uint16_t port);
void *get_in_addr(struct sockaddr *sa);

// need to free "the return pointer" with function adresse_internet_free
adresse_internet *_adresse_internet_new(sa_family_t sa_family, const int flag, const char *nom, uint16_t port) {
	adresse_internet *adr;
	struct addrinfo hints = {0}; // or : memset(&hints, 0, sizeof(struct addrinfo));
	memset(&hints, 0, sizeof(struct addrinfo));
	struct addrinfo *result;
	int s;
	char s_port[MAX_NUMBERS_PORT];
	if (snprintf(s_port, sizeof(s_port), "%d", htons(port)) < 0) {
		fprintf(stderr, "Erreur snprintf s_port : %s\n", strerror(errno));
		return NULL;
	}
	hints.ai_flags = flag;
	s = getaddrinfo(nom, s_port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "Erreur getaddrinfo : %s\n", gai_strerror(s));
		return NULL;
	}	
	size_t size = sizeof(adresse_internet);
	adr = malloc(size);
	if (adr == NULL) {
		fprintf(stderr, "Erreur malloc : %lu (size) adr\n", size);
		return NULL;
	}
	
	for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
		// Prend la 1ère adresse de la même famille même si il y en a plusieurs
		if (rp->ai_family == sa_family) {
			// printf("family : %d - data : %s\n", rp->ai_addr->sa_family, rp->ai_addr->sa_data);
			char buffer[BUFFER_SIZE];
			void *in_addr = get_in_addr(rp->ai_addr);
			if (inet_ntop(rp->ai_family, in_addr, buffer, sizeof(buffer)) == NULL) {
				fprintf(stderr, "Erreur inet_ntop\n");
				return NULL;
			}
			// printf("Network address: %s - , nom canonique : result->ai_canonname\n", buffer);
			// printf("Port : %d\n", ntohs(((struct sockaddr_in *) (rp->ai_addr))->sin_port));
			// printf("Sauvegarde dans la structure le nom : %s et le port %s\n", adr->nom, adr->service);
			memcpy(&(adr->sock_addr), result->ai_addr, result->ai_addrlen);
			int errnum;
			memset(&(adr->nom), '\0', sizeof(adr->nom));
			if (nom != NULL) {
				if ((errnum = snprintf(adr->nom, sizeof(adr->nom), "%s", nom)) < 0) {
					fprintf(stderr, "Erreur snprintf\n");
					return NULL;
				}
			}
			if ((errnum = snprintf(adr->service, sizeof(adr->service), "%d", port)) < 0) {
				fprintf(stderr, "Erreur snprintf\n");
				return NULL;
			}
			freeaddrinfo(result);
			// free of adr -> adresse_internet_free(adresse_internet *adresse)
			return adr;
		}
	}
	return NULL;
}

void *get_in_addr(struct sockaddr *sa) {
	switch (sa->sa_family) {
		case (AF_INET) : {
			struct sockaddr_in *sain = (struct sockaddr_in *) sa;
			return &sain->sin_addr;
		}
		case (AF_INET6) : {
			struct sockaddr_in6 *sain6 = (struct sockaddr_in6 *) sa;
			return &sain6->sin6_addr;
		}
		default:
			return NULL;
	}
}
	
adresse_internet *adresse_internet_new(const char *nom, uint16_t port) {
	return _adresse_internet_new(DEFAULT_SA_FAMILY, 0, nom, port);
}
	
adresse_internet *adresse_internet_any(uint16_t port) {
	return _adresse_internet_new(DEFAULT_SA_FAMILY, AI_PASSIVE, NULL, port);
}
	
adresse_internet *adresse_internet_loopback(uint16_t port) {
	return _adresse_internet_new(DEFAULT_SA_FAMILY, 0, NULL, port);
}
	
void adresse_internet_free(adresse_internet *adresse) {
	if (adresse != NULL) {
		free(adresse);
	}
}
/*
int adresse_internet_get_info(adresse_internet *adresse,
    char *nom_dns, int taille_dns, char *nom_port, int taille_port) {
	// pas à faire
	return 0;
}
*/

int adresse_internet_get_ip(const adresse_internet *adresse, char *ip,
    int taille_ip) {
	const char *r;
	switch (adresse_internet_get_domain(adresse)) {
		case AF_INET : {
			struct sockaddr_in *sin = (struct sockaddr_in *) &adresse->sock_addr;
			r = inet_ntop(sin->sin_family, (const void *) &sin->sin_addr, ip, (socklen_t) taille_ip);
			break;
		}
		case AF_INET6 : {
			struct sockaddr_in6 *sin = (struct sockaddr_in6 *) &adresse->sock_addr;
			r = inet_ntop(sin->sin6_family, (const void *) &sin->sin6_addr, ip, (socklen_t) taille_ip);
			break;
		}
		default :
			fprintf(stderr, "inet_ntop: type d'adresse non supporté\n");
			return -1;
	}
	if (r == NULL) {
		fprintf(stderr, "Erreur inet_ntop\n");
		return -1;
	}
	return 0;
}
	
uint16_t adresse_internet_get_port(const adresse_internet *adresse) {
	if (adresse == NULL) {
		return 0;
	}
	return (uint16_t) atoi(adresse->service);
}
	
int adresse_internet_get_domain(const adresse_internet *adresse) {
	if (adresse == NULL) {
		return -1;
	}
	return adresse->sock_addr.ss_family;
}

// need to free "adresse_internet *adresse" when call
int sockaddr_to_adresse_internet(const struct sockaddr *addr, adresse_internet *adresse) {
	if (addr == NULL) {
		return -1;
	}
	memcpy(&adresse->sock_addr, addr, sizeof(*addr));
	if (snprintf(adresse->nom, sizeof(char) * 2, "%s", "") < 0) {
		fprintf(stderr, "Erreur snprintf : adresse->nom\n");
		return -1;
	}
	if (snprintf(adresse->service, sizeof(char) * 2, "%s", "") < 0) {
		fprintf(stderr, "Erreur snprintf : adresse->service\n");
		return -1;
	}
	return 0;
}
	
// need to free "adresse_internet *adresse" when call
int adresse_internet_to_sockaddr(const adresse_internet *adresse, struct sockaddr *addr) {
	if (adresse == NULL) {
		return -1;
	}
	memcpy(addr, &adresse->sock_addr, sizeof(*adresse));
	return 0;
}
	
int adresse_internet_compare(const adresse_internet *adresse11, const adresse_internet *adresse22) {
	if (adresse11 == NULL || adresse22 == NULL) {
		return -1;
	}
	if (
		adresse11->sock_addr.ss_family == adresse22->sock_addr.ss_family 
		&& strncmp(adresse11->nom, adresse22->nom, strlen(adresse22->nom)) == 0
		&& strncmp(adresse11->service, adresse22->service, strlen(adresse22->service)) == 0 
	) {
		return 1;
	}
	return 0;
}
// variables in argument are already allocated
int adresse_internet_copy(adresse_internet *adrdst, const adresse_internet *adrsrc) {
	if (adrsrc == NULL || adrdst == NULL) {
		return -1;
	}
	memcpy(adrdst, adrsrc, sizeof(adresse_internet));
	return 0;
}
