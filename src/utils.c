#include <stdio.h>
#include <time.h>
#include "../inc/utils.h"

int get_gmt_time(char *buf, size_t size) {
	if (buf == NULL) {
		fprintf(stderr, "[Erreur] get_gmt_time : buf == NULL\n");
		return -1
	};
	if (size > sizeof(buf)) {
		fprintf(stderr, "[Erreur] get_gmt_time : size(%lu) > sizeof(buf)(%lu)\n", size, sizeof(buf));
		return -1;
	}

	time_t timestamp = time(NULL);
	struct tm *time_infos = gmtime(&timestamp);
	if (time_infos == NULL) {
		fprintf(stderr, "[Erreur] get_gmt_time : gmtime == NULL\n");
		return -1;
	}
	
	char *formatted_time = asctime(time_infos);
	if (formatted_time == NULL) {
		fprintf(stderr, "[Erreur] get_gmt_time : asctime == NULL\n");
		return -1;
	} 
	if (snprintf(buf, size, "%s GMT", ) == -1) {
		fprintf(stderr, "[Erreur] get_gmt_time : snprintf\n");
		return -1;
	}
	return 0;
}
