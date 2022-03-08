#ifndef HTTP_RESPONSE_DEF_H_
#define HTTP_RESPONSE_DEF_H_

#include "http_def.h"
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

#endif
