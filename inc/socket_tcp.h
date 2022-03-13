#ifndef SOCKET_TCP_H_
#define SOCKET_TCP_H_
#include <stdint.h>
#include <socket_tcp_type.h>
#define SIZE_QUEUE 10 

/**
 * alloue et initialise la socket et renvoie un pointeur vers la zone mémoire allouée et initialisée.
 * Valeur de retour : l’adresse de la socket en cas de succès, NULL sinon.
*/
extern socket_tcp *init_socket_tcp(void);
/**
 * Se connecte sur une machine distante dont l’adresse et le port sont donnés en paramètre.
 * La fonction retourne 0 si la connexion est établie, −1 sinon. 
 * Cette fonction devra mettre à jour les champs de la structure socket_tcp passée en paramètre.
*/
extern int connect_socket_tcp(socket_tcp *s, const char *adresse, uint16_t port);
/**
 * permet de créer une socket d’écoute et de l’attacher à l’adresse et au port donnés en paramètres. 
 * Si adresse est NULL, la socket s écoute sur toutes les interfaces.
 * De plus, cette fonction devra créer une file de connexions en entrée de taille SIZE_QUEUE sur socket_tcp. 
 * La constante SIZE_QUEUE est défini dans le fichier d’entête socket_tcp.h. 
 * La fonction retourne -1 si la socket n’est pas créée ou si l’attachement n’a pu avoir lieu, 0 si tout s’est bien passé.
*/
extern int ajoute_ecoute_socket_tcp(socket_tcp *s, const char *adresse, uint16_t port);
/**
 * attend une connexion sur la socket d’écoute s, et met à jour la socket de service service lorsque
 * la connexion est établie. Cette fonction est bloquante jusqu’à ce qu’une connexion soit établie. 
 * Elle retourne -1 en cas d’erreur, 0 si tout s’est bien passé.
*/
extern int accept_socket_tcp(const socket_tcp *s, socket_tcp *service);
/**
 * écrit sur s un bloc d’octets buffer de taille length et retourne la taille des données écrites.
*/
extern ssize_t write_socket_tcp(const socket_tcp *s, const void *buffer, size_t length);
/**
 * lit sur s un bloc d’octets de taille au plus length dans buffer et retourne la taille des données réellement lues.
*/
extern ssize_t read_socket_tcp(const socket_tcp *s, const void *buffer, size_t length);
/**
 * ferme la connexion dans les deux sens et libère l’espace éventuellement alloué par la socket_tcp.
*/
extern int close_socket_tcp(socket_tcp *s);

#endif
