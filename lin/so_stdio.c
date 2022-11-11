#include "so_stdio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#define BUFFSIZE 4096

// comentariile in engleza sunt doar definitii de pe net a ceea ce trebuie sa returneze functia (luate de pe https://www.tutorialspoint.com/c_standard_library/)

struct _so_file
{
    char buffer[BUFFSIZE];
    int fd;
    // int filesize; //neok- scriu mereu, se modif mereu; mai bn pun o functie
    int actualpoz; // pt tell/verificare EOF - poz efectiva din fis
    int bufferpoz; // presupun ca am nevoie pt cand lucrez cu bufferul -- mai mult l-am folosit ca dimensiune a bufferului, dar mrg
    int bufferlen;
    char tip[3]; // verificare sa nu fac write pe un fis readonly chestii
    int hasErr;
    char last_op; //'w' - scriere, 'r' - citire
    int pid;
    int eof;
};

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
    printf("Am intrat in fopen\n");
    if (pathname == NULL || mode == NULL)
    {
        return NULL;
    }
    if ((strcmp(pathname, "") == 0) || ((strcmp(mode, "w") != 0) && (strcmp(mode, "r") != 0) && (strcmp(mode, "a") != 0) && (strcmp(mode, "w+") != 0) && (strcmp(mode, "r+") != 0) && (strcmp(mode, "a+") != 0)))
    {
        return NULL;
    }

    int fd = -1; // poate am ratat vreun caz pentru if-uri si il fortez sa-mi intre la verificarea (fd<0)
    SO_FILE *FILE = (SO_FILE *)malloc(sizeof(SO_FILE));
    if (FILE == NULL)
    {
        free(FILE);
        return NULL; // This function returns a FILE pointer
    }                //  Otherwise, NULL is returned and the global variable errno is set to indicate the error.
    /* De aici e cazul in regula*/
    memset(FILE->buffer, 0, BUFFSIZE); // am zeroizat zona bufferului (y)
    // FILE->filesize = lseek(FILE->fd, 0, SEEK_END); //bag fctia la nevoie
    FILE->actualpoz = 0;
    FILE->bufferpoz = 0;
    FILE->bufferlen = 0;
    FILE->hasErr = 0;
    FILE->last_op = 0;
    FILE->eof = 0;
    FILE->pid = -1;
    strcpy(FILE->tip, mode);
    if (strcmp(mode, "r") == 0)
    {
        fd = open(pathname, O_RDONLY);
    }
    else if (strcmp(mode, "r+") == 0)
    {
        fd = open(pathname, O_RDWR);
    }
    else if (strcmp(mode, "w") == 0)
    {
        fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }
    else if (strcmp(mode, "w+") == 0)
    {
        fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, 0644);
    }
    else if (strcmp(mode, "a") == 0)
    {
        fd = open(pathname, O_APPEND | O_CREAT | O_WRONLY, 0644);
    }
    else if (strcmp(mode, "a+") == 0)
    {
        fd = open(pathname, O_APPEND | O_CREAT | O_RDWR, 0644);
    }
    FILE->fd = fd;
    if (FILE->fd < 0)
    {
        free(FILE);
        return NULL;
    }
    return FILE;
}

int so_fclose(SO_FILE *stream) // This method returns zero if the stream is successfully closed. On failure, EOF is returned.
{
    // sa nu uit - TO DO: flush
    if (stream != NULL)
    {
        // printf("%d", stream->fd);
        int ret_flush = 0;
        if (stream->last_op == 'w')
            ret_flush = so_fflush(stream);
        int isOK = close(stream->fd); // close() returns zero on success. On error, -1 is returned, and errno is set appropriately.

        if (isOK < 0 || ret_flush < 0)
        {
            stream->hasErr = 1;
            // free(stream);
            return SO_EOF;
        }
        free(stream);
    }
    stream = NULL;
    return 0;
}

int so_fileno(SO_FILE *stream)
// If successful, fileno() returns the file descriptor number associated...
// If unsuccessful, fileno() returns -1 and sets errno
{
    if (stream != NULL)
        if (stream->fd > 0)
            return stream->fd;
    return -1;
}

int so_feof(SO_FILE *stream) // This function returns a non-zero value when End-of-File indicator
                             // associated with the stream is set, else zero is returned. <=> 1 cand e eof, 0 cand nu
{
    /*if ((stream->actualpoz < filesize(stream->fd)) || (stream->last_op == 'w' && stream->bufferpoz <= 0) || (stream->last_op == 'r' && stream->actualpoz < filesize(stream->fd)))
        return 1;
    return 0;*/
    if (stream == NULL)
        return SO_EOF;
    return stream->eof;
}

int so_ferror(SO_FILE *stream) //  If the error indicator associated with the stream was set,
                               // the function returns a non-zero value else, it returns a zero value.
{
    if (stream == NULL)
        return SO_EOF;
    return stream->hasErr; // singura eroare posibila
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
    if (stream == NULL)
        return -1;

    int res;
    if (stream->last_op == 'r')
    {
        memset(stream->buffer, 0, BUFFSIZE); // poate nici nu e nevoie de asta daca ma bazez pe buffpoz
        stream->bufferpoz = 0;
        stream->bufferlen = 0;
    }
    else if (stream->last_op == 'w')
    {
        int r = so_fflush(stream);
        if (r < 0)
        {
            stream->hasErr = 1;
            return -1;
        }
    }

    res = lseek(stream->fd, offset, whence);
    if (res < 0)
    {
        stream->hasErr = 1;
        return SO_EOF;
    }

    stream->eof = 0;
    stream->actualpoz = res;
    return 0; //"returns zero if successful, or else it returns a non-zero value"
}

long so_ftell(SO_FILE *stream) // returns the current value of the position indicator
                               // If an error occurs, -1L is returned, and the global variable errno is set to a positive value.
{
    if (stream != NULL)
        return stream->actualpoz;
    return -1;
}

int so_fflush(SO_FILE *stream)
{
    // posibile probleme: stream null; buffer gol; sa vreau sa fac flush pe fis fara w; problema la write maybe? - ret 0 daca ok, EOF altfel
    if (stream != NULL)
    {
        // if() sa vad ce verif la poz in buffer
        if (strcmp(stream->tip, "r") == 0 || stream->last_op != 'w')
        {
            // stream->bufferpoz=0;
            stream->hasErr = 1;
            return SO_EOF;
        }
        int fin = stream->bufferlen;
        int start = 0;
        while (start < fin)
        {
            ssize_t bytes_scrisi_acm = write(stream->fd, stream->buffer + start, fin - start);
            if (bytes_scrisi_acm < 0)
            {
                stream->hasErr = 1;
                printf("so_fflush: Err bytes_scrisi_acum: %s\n", strerror(errno));
                return SO_EOF;
            }
            start += bytes_scrisi_acm;
        }
        stream->last_op = 0;
        stream->bufferpoz = 0;
        stream->bufferlen = 0;
        memset(stream->buffer, 0, BUFFSIZE);
        return 0;
    }
    stream->hasErr = 1;
    return SO_EOF;
}

int so_fgetc(SO_FILE *stream)
{
    if (stream == NULL)
        return SO_EOF;
    if (strcmp(stream->tip, "w")==0 || stream->eof == 1)
    {
        printf("Err fgetc, nu se permite operatia");
        stream->hasErr = 1;
        return SO_EOF;
    }
    int char_to_return;
    if (stream->bufferlen == 0 || stream->bufferpoz == stream->bufferlen) // stream->actualpoz!=filesize(stream)&&filesize(stream)>0) //am probleme la popen daca nu pun ultima conditie
    {
        int nr_bytes_cititi_acum;
        nr_bytes_cititi_acum = read(stream->fd, stream->buffer, BUFFSIZE);
        if (nr_bytes_cititi_acum <= 0)
        {
            if (nr_bytes_cititi_acum == 0)
                stream->eof = 1;
            else
                stream->hasErr = 1;
            // printf("so_fgetc: Err stream->bufferpoz: %s\n", strerror(errno));
            return SO_EOF;
        }
        stream->bufferlen = nr_bytes_cititi_acum;
        stream->bufferpoz = 0;
    }
    char_to_return = stream->buffer[stream->bufferpoz++];
    // if(stream->bufferpoz==0) stream->eof=SO_EOF; //NU E OKE
    stream->actualpoz++; // O UITASEM!!!
    stream->last_op = 'r';
    return char_to_return;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    // ptr - pointer vector de unde incep sa pun ce citesc
    // size - dimens unui elem din ptr[]
    // nmemb - nr max elem de dimens size de citit
    if (strcmp(stream->tip, "w")==0)
    {
        printf("CItire la wirte only!");
        stream->hasErr = 1;
        return SO_EOF;
    }
    if(size<=0 || nmemb<=0)
    {
        printf("Param incorecti la fread!");
        stream->hasErr = 1;
        return SO_EOF;
    }
    int lim = size * nmemb;
    int i;
    for (i = 0; i < lim; i++)
    {
        int c = so_fgetc(stream);
        if (c < 0 && stream->eof == 1)
            return i / size;
        else if (c < 0 && stream->hasErr == 1)
            return SO_EOF;
        // memcpy(ptr + i, &c, 1);
        *(unsigned char *)ptr = (unsigned char)c;
        ptr++;
    }
    stream->last_op = 'r';
    return i / size;
}

int so_fputc(int c, SO_FILE *stream)
{
    if (stream == NULL)
    {
        return SO_EOF;
    }

    if(strcmp(stream->tip,"r")==0)
    {
        stream->hasErr=1;
        return SO_EOF;
    }
    if (stream->bufferpoz == BUFFSIZE)
    {
        int nok = so_fflush(stream);
        if (nok < 0)
        {
            stream->hasErr = 1;
            // stream->eof = SO_EOF;
            printf("so_fputc + fflush: Err fflush: %s\n", strerror(errno));
            return SO_EOF;
        }
    }
    stream->buffer[stream->bufferpoz++] = (unsigned char)c;
    stream->bufferlen++;
    stream->actualpoz++;
    stream->last_op = 'w';
    return c;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream) // param ca la fread
{
    if (stream == NULL || strcmp(stream->tip,"r")==0 || size <= 0 || nmemb <= 0)
    {
        stream->hasErr = 1;
        return SO_EOF;
    }
    int lim = size * nmemb;
    int i;
    stream->last_op = 'w';
    for (i = 0; i < lim; i++)
    {
        // int c;
        // memcpy(&c, ptr + i, 1);
        int c = *((int *)(ptr + i));
        int ok = so_fputc(c, stream);
        if (ok < 0 && stream->hasErr == 1)
        {
            return SO_EOF;
        }
    }
    return i / size;
}

#define BUFFER_PIPE 32
SO_FILE *so_popen(const char *command, const char *type)
{
    printf("AM intrat in popen\n");
    int ret;
    if ((strcmp(type, "r") != 0) && (strcmp(type, "w") != 0))
    {
        printf("Popen: argument type invalid\n");
        return NULL;
    }
    // Alocam un vector de 2 elemente int in care se vor scrie
    // descriptorii celor doua capete ale pipe-ului
    int pipe_fd[2];

    // Obtinem cei doi descriptori ai pipe-ul.
    // [0] - capatul de citire
    // [1] - capatul de scriere

    int child = 0;  // <=> READ_END
    int parent = 1; // <=> WRITE_END

    ret = pipe(pipe_fd);
    if (ret < 0)
    {
        perror("error on creating pipe: ");
        exit(-1);
    }
    // r -> ret fis read-only; deci pentru r, am r la copil si w la parinte; invers pt w
    if (!strcmp(type, "r"))
    {
        child = 1;
        parent = 0;
    }

    pid_t pid;
    pid = fork();
    if (pid < 0)
    {
        printf("ERR PID NEG???");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return NULL;
    }

    // Daca suntem in procesul copil
    if (pid == 0)
    {
        printf("Am proc copil\n");
        // Procesul copil a mostenit descriptorii pipe-ului
        // inchidem capatul nefolosit
        close(pipe_fd[parent]);

        if (!strcmp(type, "w"))
        {
            int rc = dup2(pipe_fd[child], STDIN_FILENO);
            if (rc < 0)
                return NULL;
        }
        if (!strcmp(type, "r"))
        {
            int rc = dup2(pipe_fd[child], STDOUT_FILENO);
            if (rc < 0)
                return NULL;
        }
        char *argv[4] = {"sh", "-c", NULL, NULL};
        argv[2] = (char *)command;
        printf("\nIntru in execvp copil\n");
        execvp("sh", argv);
        return NULL; // daca esueaza execul
    }

    // Daca suntem in procesul parinte
    // if (pid > 0)
    //{
    printf("Am proc parinte\n");
    SO_FILE *file = malloc(sizeof(SO_FILE));
    close(pipe_fd[child]);
    if (file == NULL)
        return NULL;
    file->fd = pipe_fd[parent];
    memset(file->buffer, 0, BUFFSIZE); // am zeroizat zona bufferului (y)
    file->actualpoz = 0;
    file->bufferpoz = 0;
    file->pid = pid;
    strcpy(file->tip, type);
    file->bufferlen = 0;
    file->hasErr = 0;
    file->last_op = 0;
    file->eof = 0;
    return file;
    //}
}

int so_pclose(SO_FILE *stream)
{
    if (stream != NULL)
    {
        int status;
        int close_status = so_fclose(stream);
        int r = waitpid(stream->pid, &status, 0);

        if (r < 0)
            return -1;
        if (WIFEXITED(status) && close_status == 0)
            return 0;
    }
    return -1;
}

// #include "_test/src/large_file.h"

// int main()
// {

//     SO_FILE *f;
// 	unsigned char *tmp;
// 	int ret;
// 	char *test_work_dir;
// 	char fpath[256];
// 	char cmd[128];
// 	int chunk_size = 200;
// 	int total;
//     int target_fd;

//     f = so_fopen("/home/lori/TemaMare1/_test/huge_file", "w");
//     // FAIL_IF(!f, "Couldn't open file: %s\n", fpath);

//     target_fd = so_fileno(f);
//     printf("%d", target_fd);

//     //ret = so_fwrite(buf, 1, buf_len, f);
//     total = 0;
//     tmp = malloc(buf_len);
// 	while (!so_feof(f)) {
// 		ret = so_fread(&tmp[total], 1, chunk_size, f);

// 		total += ret;
// 	}
//     ret = so_ferror(f);
//     ret = so_pclose(f);
// 	if(ret != 0)
//     printf( "Incorrect return value for so_pclose: got %d, expected %d\n", ret, 0);

//     //if (ret == 0)
//     //    printf("Incorrect return value for so_ferror: got %d, expected != 0\n", ret);

//     so_fclose(f);
// }


int main()
{
    SO_FILE *fd;
    fd = so_fopen("file.txt", "xyz");
    // fp = so_popen("pwd", "r");
    // char cuv[10];
    // so_fread(cuv, 1, 10, fp);
    int target_fd = so_fileno(fd);
    char c = so_fgetc(fd);
    printf("%c", c);
    c = so_fgetc(fd);
    printf("%c", c);
    c = so_fgetc(fd);
    printf("%c", c);
    // printf("*%s*", cuv);
    //  int status = so_pclose(fp);
    //   if (status == -1)
    //   {
    //       printf("Error reported by pclose()");
    //   }
    so_fclose(fd);
    return 0;
}

// int main()
// {
//     // merge ok si asta
//     SO_FILE *fp;
//     // char c;
//     fp = so_fopen("file.txt", "a+");
//     // while (1)
//     // {
//     //     c = so_fgetc(fp);
//     //     if (so_feof(fp))
//     //     {
//     //         break;
//     //     }
//     //     printf("%c", c);
//     // }

//     int status;
//     char path[10];
//     fp = so_popen("pwd", "r");
//     if (fp == NULL)
//         printf(" Handle error from popen");
//     printf("Am in buffer: %s\n", fp->buffer);
//     status = so_pclose(fp);
//     if (status == -1)
//     {
//         printf("Error reported by pclose()");
//     }
//     so_fclose(fp);
//     return (0);
// }

// int main()
// {
//     /*SO_FILE *myf = so_fopen("text.txt", "a+");
//     // printf("%d", so_feof(myf));
//     // strcpy(myf->buffer,"naspa");
//     // myf->bufferpoz=strlen("naspa")-1;
//     //char c = so_fgetc(myf);
//     //so_fputc('$', myf);
//     char cuv[20];
//     int size = so_fread(cuv, 1, 100, myf);
//     // printf("%s", cuv);
//     so_fwrite(cuv, 1, strlen(cuv), myf);
//     // printf("\nAm extras caracterul %c\nBuffer: %s",c, myf->buffer);
//     // so_fflush(myf);
//     // printf("%d", myf->bufferpoz);
//     so_fclose(myf);*/

//     /*Exemplu de main luat de pe net si pun so la toate fctiile: //A MEEEEERS*/
//     SO_FILE *fp;

//     fp = so_fopen("file.txt", "w+");
//     char string[]="This is tutorialspoint.com";
//     so_fwrite(string,1,strlen(string), fp);
//     /* for(int i=0;i<strlen(string);i++)
//     {
//         so_fputc(string[i], fp);
//     } */

//     so_fseek(fp, 7, SEEK_SET);
//     strcpy(string," C Programming Language");
//     so_fwrite(string,1,strlen(string), fp);
//     /*
//     for(int i=0;i<strlen(string);i++)
//     {
//         so_fputc(string[i], fp);
//     } */
//     so_fclose(fp);

//     return 0;
// }
