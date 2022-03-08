#ifndef HTTP_DEF_H_
#define HTTP_DEF_H_

// Mapping enum with string : https://www.linkedin.com/pulse/mapping-enum-string-c-language-sathishkumar-duraisamy
// Test : printf("%s", status_names[OK]);

#define HTTP_PROTOCOL "HTTP/1.1"

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

#endif
