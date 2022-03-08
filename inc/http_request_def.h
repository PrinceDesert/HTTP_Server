#ifndef HTTP_REQUEST_DEF_H_
#define HTTP_REQUEST_DEF_H_

#include "http_def.h"

/*----------------*/
/* Client Request */
/*----------------*/
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
