#ifndef SOCKET_TCP_TYPE_H_
#define SOCKET_TCP_TYPE_H_
#include "adresse_internet_type.h"
	typedef struct {
		int socket_fd;
		int is_bind;
		int is_listening;
		int is_connected;
		adresse_internet *local;
		adresse_internet *remote;
	} socket_tcp;
#endif
