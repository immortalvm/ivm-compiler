/* Some useful functions */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>

/* Generate a temporary file; filename is
   allocated and returned; template is a
   const string to which a suffix is added
   ("_sXXXXXX") as needed by mktemp() */
char *gen_tempfile(const char *template){
    char *tmpdir; DIR *dirp;
    if ((tmpdir = getenv("TMPDIR")) && (dirp = opendir(tmpdir)))
        closedir(dirp);
    else if ( (tmpdir = "/tmp") && (dirp = opendir(tmpdir)))
        closedir(dirp);
    else
        tmpdir = ".";

    char *filename = (char*)malloc(sizeof(char)*(strlen(template) + strlen(tmpdir)+16));
    sprintf(filename, "%s/%s_sXXXXXX", tmpdir, template);
    int fno = mkstemp(filename);
    if (-1 == fno) {
        fprintf(stderr, "Error opening temporary file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }
    close(fno);
    return filename;
}

void file_put_contents(char *filename, FILE* out_file){
    char *buff[1024];
    size_t s;
    FILE *fd = fopen(filename, "r");
    if (fd){
        while (s = fread(buff, 1, 1024, fd)) {
            fwrite(buff, 1, s, out_file);
        }
    } else {
        fprintf(stderr, "Error opening %s\n", filename);
        exit(EXIT_FAILURE);
    }
}

void stdin_to_file(char *filename){
    char *buff[1024];
    size_t s;
    FILE *out_file = fopen(filename, "w");
    if (out_file){
        while (s = fread(buff, 1, 1024, stdin)) {
            fwrite(buff, 1, s, out_file);
        }
    } else {
        fprintf(stderr, "Error opening %s\n", filename);
        exit(EXIT_FAILURE);
    }
    fclose(out_file);
}

/* Trim functions from the internet */
char *ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}

char *rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}

char *trim(char *s)
{
    return rtrim(ltrim(s));
}


