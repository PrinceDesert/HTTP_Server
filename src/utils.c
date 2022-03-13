#include <stdio.h>
#include <string.h>
#include <time.h>
#include <utils.h>

int get_gmt_time(char *buf, size_t size) {
	if (buf == NULL) {
		fprintf(stderr, "[Erreur] get_gmt_time : buf == NULL\n");
		return -1;
	};
	
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
	formatted_time[strcspn(formatted_time, "\n")] = '\0';
	if (snprintf(buf, size, "%s GMT", formatted_time) == -1) {
		fprintf(stderr, "[Erreur] get_gmt_time : snprintf\n");
		return -1;
	}
	return 0;
}

/**
 * Source : https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
 * Example :
 * printf("%s\n", get_filename_ext("test.tiff"));
 * printf("%s\n", get_filename_ext("test.blah.tiff"));
 * printf("%s\n", get_filename_ext("test."));
 * printf("%s\n", get_filename_ext("test"));
 * printf("%s\n", get_filename_ext("..."));
*/
const char *get_filename_ext(const char *filename) {
	const char *dot = strrchr(filename, '.');
	if (!dot || dot == filename) return "";
	return dot + 1;
}
