#include <stdio.h>

int main(int argc, char* argv[]){
    
    if (argc > 0 && argv){
        printf("%s found %d arguments\n", argv[0], argc);
        for (int i=0; i<argc; i++){
                printf("\targv[%d]=%s\n", i, argv[i]);
        }
        if (argv[argc] == NULL) 
            printf("argv[%d] is null\n", argc);
         else
            printf("argv[%d] NOT null!\n", argc);
        return argc;
    } else {
        printf("-----Hello!, no arguments found\n");
    }
    return 0;
}
