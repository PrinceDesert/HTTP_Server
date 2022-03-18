#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <adresse_internet.h>

#define DOMAIN_NAME "univ-rouen.fr"
#define PORT 8080

int main() {
	printf("========== TEST : adresse_internet_new ==========\n");
	adresse_internet *adr_new = adresse_internet_new(DOMAIN_NAME, PORT);
	if (adr_new == NULL) {
		fprintf(stderr, "Erreur adr_new");
		exit(EXIT_FAILURE);
	} else {
		printf("famille : %u\n", adr_new->sock_addr.ss_family);
		printf("nom : %s\n", adr_new->nom);
		printf("service : %s\n",  adr_new->service);
	}
	
	
	printf("========== TEST : adresse_internet_any ==========\n");
	adresse_internet *adr_any = adresse_internet_any(PORT);
	if (adr_any == NULL) {
		fprintf(stderr, "Erreur adr_any");
		exit(EXIT_FAILURE);
	} else {
		printf("famille : %u\n", adr_any->sock_addr.ss_family);
		printf("nom : %s\n", adr_any->nom);
		printf("service : %s\n",  adr_any->service);
	}
	
	printf("========== TEST : adresse_internet_loopback ==========\n");
	adresse_internet *adr_loopback = adresse_internet_loopback(PORT);
	if (adr_loopback == NULL) {
		fprintf(stderr, "Erreur adr_loopback");
		exit(EXIT_FAILURE);
	} else {
		printf("famille : %u\n", adr_loopback->sock_addr.ss_family);
		printf("nom : %s\n", adr_loopback->nom);
		printf("service : %s\n",  adr_loopback->service);
	}
	
	printf("========== TEST : adresse_internet_get_ip ==========\n");
	const char ip[20];
	if (adresse_internet_get_ip(adr_new, (char *) ip, (int) sizeof(ip)) != 0) {
		fprintf(stderr, "Erreur adresse_internet_get_ip\n");
	} else {
		printf("ip de %s : %s\n", DOMAIN_NAME, ip);
	}
	
	printf("========== TEST : adresse_internet_get_port ==========\n");
	uint16_t port = adresse_internet_get_port(adr_new);
	printf("port de %s : %d\n", DOMAIN_NAME, (int) port);
	
	printf("========== TEST : sockaddr_to_adresse_internet ==========\n");
	struct sockaddr *addr = malloc(sizeof(struct sockaddr));
	addr->sa_family = AF_INET;
	adresse_internet *ai = malloc(sizeof(adresse_internet));
	if (sockaddr_to_adresse_internet(addr, ai) == -1) {
		fprintf(stderr, "Erreur sockaddr_to_adresse_internet\n");
	} else {
		printf("famille : %u\n", ai->sock_addr.ss_family);
		printf("nom : %s\n", ai->nom);
		printf("service : %s\n",  ai->service);
	}
	free(addr);
	free(ai);
	
	printf("========== TEST : adresse_internet_to_sockaddr ==========\n");
	addr = malloc(sizeof(struct sockaddr));
	if (adresse_internet_to_sockaddr(adr_new, addr) == -1) {
		fprintf(stderr, "Erreur adresse_internet_to_sockaddr\n");
	} else {
		printf("family : %d\n", addr->sa_family);
		printf("data : %s\n", addr->sa_data);
	}
	free(addr);
	
	// adresse_internet_compare
	printf("========== TEST : adresse_internet_compare ==========\n");
	printf("compare test 1 : %d\n", adresse_internet_compare(adr_new, adr_any));
	printf("compare test 2 : %d\n", adresse_internet_compare(adr_new, adr_new));
	//adresse_internet *adr_new2 = adresse_internet_new("www.google.com", 80);
	/*printf("compare test 3 : %d\n", adresse_internet_compare(adr_new, adr_new2));*/
	//adresse_internet_free(adr_new2);
	
	// adresse_internet_copy
	
	
	adresse_internet_free(adr_new);
	adresse_internet_free(adr_any);
	adresse_internet_free(adr_loopback);
	return EXIT_SUCCESS;
}
