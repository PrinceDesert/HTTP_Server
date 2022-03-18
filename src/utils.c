#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
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

/**
	* Enlever les espaces avant et après
	* Source : https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
	* @param : la chaîne de caractères où il faut enlever les espaces avant et après
*/
void trim(char *str) {
	int i;
	int begin = 0;
	int end = ((int) strlen(str)) - 1;
	// Fait avancer le curseur du début jusqu'à ce qu'il n'y est plus d'espace au début
	while (isspace((unsigned char) str[begin])) {
		begin++;
	}
	// Fait avancer le curseur de fin jusqu'à ce qu'il n'y est plus d'espace à la fin
	while ((end >= begin) && isspace((unsigned char) str[end])) {
		end--;
	}
	// Shift all characters back to the start of the string array.
	for (i = begin; i <= end; i++) {
		str[i - begin] = str[i];
	}
	// Null terminate string.
	str[i - begin] = '\0';
}
