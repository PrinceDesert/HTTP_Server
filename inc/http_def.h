#ifndef HTTP_DEF_H_
#define HTTP_DEF_H_
#include <string.h>

// Mapping enum with string : https://www.linkedin.com/pulse/mapping-enum-string-c-language-sathishkumar-duraisamy
// Test : printf("%s", status_names[OK]);

/*-------------------*/
/* HTTP Informations */
/*-------------------*/
#define HTTP_VERSION_PROTOCOL "HTTP/1.1"
#define EMPTY_LINE "\r" // Récupèrer en affichant le code ascii de la ligne vide de http
#define MAX_SIZE_METHOD 64
#define MAX_SIZE_URL	4096
#define MAX_SIZE_HTTP_VERSION_PROTOCOL sizeof(char) * (strlen(HTTP_VERSION_PROTOCOL) + 1)


/*------------------*/
/* Header format	*/
/* name : value		*/
/*------------------*/
#define MAX_SIZE_NAME_HEADER 64
#define MAX_SIZE_VALUE_HEADER 256 
typedef struct {
	char name[MAX_SIZE_NAME_HEADER]; // Request/Response -> Date, useragent, content-type / Response -> Date, Server, Content-Type
	char value[MAX_SIZE_VALUE_HEADER]; // valeur du nom
} _header_t;
typedef _header_t header_t;

/*-------------*/
/* Mime Format */
/*-------------*/
// MIME : https://github.com/mdn/translated-content/blob/main/files/fr/web/http/basics_of_http/mime_types/common_types/index.md
#define MAX_SIZE_TYPE 64
#define MAX_SIZE_EXTENSION 32
typedef struct {
	const char type[MAX_SIZE_TYPE];				// text/plain
	const char *extension[MAX_SIZE_EXTENSION];	// .txt, tx
} _mime_type_extension_t;
typedef  _mime_type_extension_t mime_type_extension_t;

typedef enum { PLAIN = 0, HTML, JPEG, PNG, ZIP} mimes_t;
static const mime_type_extension_t mime_names[] = {
	[PLAIN]	= { "text/plain",		{".txt"} 			},
	[HTML]	= { "text/html",		{".html"} 			},
	[JPEG]	= {	"image/jpeg",		{".jpeg", ".jpg"}	},
	[PNG]	= { "image/png",		{".png"}			},
	[ZIP]	= { "application/zip",	{".zip"}			},
};

/*-----------------*/
/* Server Response */
/*-----------------*/
typedef enum { OK = 0, BAD_REQUEST, UNAUTHORIZED, HTTP_VERSION_NOT_SUPPORTED} status_t; // faire un switch case Ok : s = "200 OK"
// a décommenter lors de l'utilisateur => car unused variable
static const char *status_names[] = {
	[OK] 							= "200 OK",
	[BAD_REQUEST] 					= "400 Bad Request",
	[UNAUTHORIZED] 					= "401 Unauthorized",
	[HTTP_VERSION_NOT_SUPPORTED] 	= "505 HTTP Version Not Supported"
};

typedef enum {SERVER = 0, CONNECTION, CONTENT_TYPE, CONTENT_LENGTH, DATE_RESPONSE} response_names_t;
/*static const char *response_names[] = {
	[SERVER] = "Server",
	[CONNECTION] = "Connection",
	[CONTENT_TYPE] = "Content-Type",
	[CONTENT_LENGTH] = "Content-Length",
	[DATE_RESPONSE] = "Date"
};*/

// Nombre champ du header max pour une requête ou une réponse
#define MAX_NUMBER_HEADERS 64
// Remplissage
typedef struct {
	const char *protocol; // HTTP/1.x x = version
	status_t status_code; // 200 OK, 206 Partial Content, 404 Not Found, 401 Unauthorized, 403 Forbidden
	header_t headers[MAX_NUMBER_HEADERS]; // entêtes de la réponse, avec au max 64 champs
} _http_response;
typedef  _http_response http_response;

/*-------------------------*/
/* Client(Browser) Request */
/*-------------------------*/
typedef enum { GET = 0, /*POST*/} methods_t;
static const char *method_names[] = {
	[GET] = "GET"
	// [POST] = "POST"
};

typedef enum {DATE_REQUEST = 0, ACCEPT} request_names_t;
static const char *request_names[] = {
	[DATE_REQUEST] = "Date",
	[ACCEPT] = "Accept"
};

#endif
