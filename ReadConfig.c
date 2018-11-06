
#include "DataTypes.h"
#include "string.h"
Config config;
size_t getlinenew(char **linep, size_t *n, FILE *fp);

void read_configfile(char *fileName) {
    char *line = (char *) malloc (sizeof(char) * MAX_LINE);
    char *tempLine = (char *) malloc (sizeof(char) * MAX_LINE);
    char *temp = (char *) malloc (sizeof(char) * MAX_LINE);
    char *tok;

    size_t len = 0;
    ssize_t read;

    FILE *fp;

    if ((fp = fopen(fileName, "r")) == NULL) {
        perror ("Error to open the configuration file...");
        exit (EXIT_FAILURE);
    }

    while ((read = getlinenew(&line, &len, fp)) != -1) { //loop to read file line by line and tokenize
        strcpy (tempLine, line);
        if ((tempLine = strtok(tempLine, WHITE_SPACE CONFIG_EQUAL)) == NULL || *tempLine == 0) {
            continue;
        }
        char* line_ptr = NULL;
        if(strcmp(tempLine, "p")==0){
            tempLine = strtok(line, WHITE_SPACE CONFIG_EQUAL);
            tempLine = strtok(NULL, WHITE_SPACE CONFIG_EQUAL);
            config.p = atoi(tempLine);
        }else if(strcmp(tempLine, "n1")==0){
            tempLine = strtok(line, WHITE_SPACE CONFIG_EQUAL);
            tempLine = strtok(NULL, WHITE_SPACE CONFIG_EQUAL);
            config.n1 = atoi(tempLine);
        }else if(strcmp(tempLine, "n2")==0){
            tempLine = strtok(line, WHITE_SPACE CONFIG_EQUAL);
            tempLine = strtok(NULL, WHITE_SPACE CONFIG_EQUAL);
            config.n2 = atoi(tempLine);
        }else if(strcmp(tempLine, "b")==0){
            tempLine = strtok(line, WHITE_SPACE CONFIG_EQUAL);
            tempLine = strtok(NULL, WHITE_SPACE CONFIG_EQUAL);
            config.b = atoi(tempLine);
        }else if(strcmp(tempLine, "a1")==0){
            tempLine = strtok(line, WHITE_SPACE CONFIG_EQUAL);
            tempLine = strtok(NULL, WHITE_SPACE CONFIG_EQUAL);
            config.a1 = atoi(tempLine);
        }else if(strcmp(tempLine, "a2")==0){
            tempLine = strtok(line, WHITE_SPACE CONFIG_EQUAL);
            tempLine = strtok(NULL, WHITE_SPACE CONFIG_EQUAL);
            config.a2 = atoi(tempLine);
        }else if(strcmp(tempLine, "C")==0){
            tempLine = strtok(line, WHITE_SPACE CONFIG_EQUAL);
            tempLine = strtok(NULL, WHITE_SPACE CONFIG_EQUAL);
            config.C = atoi(tempLine);
        }else if(strcmp(tempLine, "d")==0){
            tempLine = strtok(line, WHITE_SPACE CONFIG_EQUAL);
            tempLine = strtok(NULL, WHITE_SPACE CONFIG_EQUAL);
            config.d = atoi(tempLine);
        }else if(strcmp(tempLine, "d1")==0){
            tempLine = strtok(line, WHITE_SPACE CONFIG_EQUAL);
            tempLine = strtok(NULL, WHITE_SPACE CONFIG_EQUAL);
            config.d1 = atoi(tempLine);
        }

        line = (char *) malloc (sizeof(char) * MAX_LINE);
        tempLine = (char *) malloc (sizeof(char) * MAX_LINE);

    }

    if (fp)
        fclose(fp);
    printf("p  = %d\n", config.p);
    printf("n1 = %d\n", config.n1);
    printf("n2 = %d\n", config.n2);
    printf("b  = %d\n", config.b);
    printf("a1 = %d\n", config.a1);
    printf("a2 = %d\n", config.a2);
    printf("C  = %d\n", config.C);
    printf("d  = %d\n", config.d);
    printf("d1 = %d\n", config.d1);
}

size_t _getdelim_(char **linep, size_t *n, int delim, FILE *fp){
    int ch;
    size_t i = 0;
    if(!linep || !n || !fp){
        //errno = EINVAL;
        return -1;
    }
    if(*linep == NULL){
        if(NULL==(*linep = malloc(*n=128))){
            *n = 0;
            //errno = ENOMEM;
            return -1;
        }
    }
    while((ch = fgetc(fp)) != EOF){
        if(i + 1 >= *n){
            char *temp = realloc(*linep, *n + 128);
            if(!temp){
                //errno = ENOMEM;
                return -1;
            }
            *n += 128;
            *linep = temp;
        }
        (*linep)[i++] = ch;
        if(ch == delim)
            break;
    }
    (*linep)[i] = '\0';
    return !i && ch == EOF ? -1 : i;
}

size_t getlinenew(char **linep, size_t *n, FILE *fp){
    return _getdelim_(linep, n, '\n', fp);
}
