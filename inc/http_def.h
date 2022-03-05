#ifndef HTTP_DEF_H_
#define HTTP_DEF_H_

#define HTTP_PROTOCOL "HTTP/1.1"

// FORMAT :
// Champ : valeur 
#define MAX_SIZE_NAME_HEADER 64
#define MAX_SIZE_VALUE_HEADER 256 
typedef struct {
	char name[MAX_SIZE_NAME_HEADER]; // Request -> Date, useragent, content-type / Response -> Date, Server, Content-Type
	char value[MAX_SIZE_VALUE_HEADER]; // valeur du nom
} _header_t;
typedef _header_t header_t;

// Nombre champ du header max pour une requête ou une réponse
#define MAX_SIZE_HEADERS_FIELDS 64



// Request methods
typedef enum { GET = 0, POST} method_t;
/* // a décommenter lors de l'utilisateur => car unused variable
static const char *method_names[] = {
	[GET] = "GET",
	[POST] = "POST"
};
*/
/**
 * exaple pour avoir la taille de methodes_name
int *k[2];
int i = 0;
while (k[i++] != NULL);

*/


// Response code
// Mapping enum with string : https://www.linkedin.com/pulse/mapping-enum-string-c-language-sathishkumar-duraisamy
// Test : printf("%s", status_names[OK]);
typedef enum { OK = 0, BAD_REQUEST, UNAUTHORIZED} status_t; // faire un switch case Ok : s = "200 OK"
/* // a décommenter lors de l'utilisateur => car unused variable
static const char *status_names[] = {
	[OK] 			= "200 OK",
	[BAD_REQUEST] 	= "400 Bad Request",
	[UNAUTHORIZED] 	= "401 Unauthorized"
};*/


#endif
