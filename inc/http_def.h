#ifndef HTTP_DEF_H_
#define HTTP_DEF_H_

// Mapping enum with string : https://www.linkedin.com/pulse/mapping-enum-string-c-language-sathishkumar-duraisamy
// Test : printf("%s", status_names[OK]);

#define HTTP_VERSION_PROTOCOL "HTTP/1.1"

// FORMAT :
// Champ : valeur 
#define MAX_SIZE_NAME_HEADER 64
#define MAX_SIZE_VALUE_HEADER 256 
typedef struct {
	char name[MAX_SIZE_NAME_HEADER]; // Request/Response -> Date, useragent, content-type / Response -> Date, Server, Content-Type
	char value[MAX_SIZE_VALUE_HEADER]; // valeur du nom
} _header_t;
typedef _header_t header_t;

// Nombre champ du header max pour une requête ou une réponse
#define MAX_SIZE_HEADERS_FIELDS 64

/*--------------*/
/* General Mime */
/*--------------*/
// MIME : https://github.com/mdn/translated-content/blob/main/files/fr/web/http/basics_of_http/mime_types/common_types/index.md
// à faire enum
typedef struct {
	const char type[32]; // text/plain
	const char *extension[10]; // .txt, tx // 10 extension max
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
// Response code
typedef enum { OK = 0, BAD_REQUEST, UNAUTHORIZED} status_t; // faire un switch case Ok : s = "200 OK"
/* // a décommenter lors de l'utilisateur => car unused variable
static const char *status_names[] = {
	[OK] 			= "200 OK",
	[BAD_REQUEST] 	= "400 Bad Request",
	[UNAUTHORIZED] 	= "401 Unauthorized"
};*/

typedef enum {SERVER = 0, CONNECTION} response_names_t;
/*static const char *response_names[] = {
	[SERVER] = "Server",
	[CONNECTION] = "Connection"
};*/

/*-------------------------*/
/* Client(Browser) Request */
/*-------------------------*/
// Request methods
typedef enum { GET = 0, POST} methods_t;
// a décommenter lors de l'utilisateur => car unused variable

static const char *method_names[] = {
	[GET] = "GET",
	[POST] = "POST"
};
typedef enum {DATE = 0, ACCEPT} request_names_t;

static const char *request_names[] = {
	[DATE] = "Date",
	[ACCEPT] = "Accept"
};

#endif
