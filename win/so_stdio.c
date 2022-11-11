// shift alt f - identarea frumoasa pt win
#define DLL_EXPORTS
#include "so_stdio.h"
#include <string.h>
#include <stdio.h> //de sters
#include <errno.h>
#define BUFFSIZE 4096

// comentariile in engleza sunt doar definitii de pe net a ceea ce trebuie sa returneze functia (luate de pe https://www.tutorialspoint.com/c_standard_library/)

struct _so_file
{
    char buffer[BUFFSIZE];
    HANDLE fh; // daca la linux pun fd file descriptor, aici pun fh file handler
    // int filesize; //neok- scriu mereu, se modif mereu; mai bn pun o functie
    int actualpoz; // pt tell/verificare EOF - poz efectiva din fis
    int bufferpoz; // presupun ca am nevoie pt cand lucrez cu bufferul -- mai mult l-am folosit ca dimensiune a bufferului, dar mrg
    int bufferlen;
    char tip[3]; // verificare sa nu fac write pe un fis readonly chestii
    int hasErr;
    char last_op; //'w' - scriere, 'r' - citire
    int eof;
    HANDLE processHandle;
};
/*
int filesize(HANDLE fh)
{
    // return lseek(fh, 0, SEEK_END);
    if (fh != INVALID_HANDLE_VALUE)
    {
        DWORD FileSize = 0;
        FileSize = GetFileSize(fh, NULL);
        if (FileSize == INVALID_FILE_SIZE)
        {
            printf("error %lu:  GetFileSize failed.\n", GetLastError());
            return 0;
        }
        return FileSize;
    }
    return 0;
}*/

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
    HANDLE h;
    if (strcmp(mode, "r") == 0)
    {
        h = CreateFileA(
            pathname,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_READONLY,
            NULL);
    }
    else if (strcmp(mode, "r+") == 0)
    {
        h = CreateFileA(
            pathname,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }
    else if (strcmp(mode, "w") == 0)
    {
        h = CreateFileA(
            pathname,
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }
    else if (strcmp(mode, "w+") == 0)
    {
        h = CreateFileA(
            pathname,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            /* file exists -> overwrite, else -> create */
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }
    else if (strcmp(mode, "a") == 0)
    {
        h = CreateFileA(
            pathname,
            FILE_APPEND_DATA, // GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }
    else if (strcmp(mode, "a+") == 0) //fun fact: la open_always cand deschid fis deja existent primesc getlasterror() 183 dar functia e succesful
    {
        h = CreateFileA(
            pathname,
            GENERIC_READ | FILE_APPEND_DATA, // GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }
    if (h == INVALID_HANDLE_VALUE)
    {
        printf("Create error\n");
        return NULL;
    }
    else
    {
        SO_FILE *FILE = (SO_FILE *)malloc(sizeof(SO_FILE));
        if (!FILE)
            return NULL; // This function returns a FILE pointer
                         //  Otherwise, NULL is returned and the global variable errno is set to indicate the error.
        /* De aici e cazul in regula*/
        FILE->fh = h;
        memset(FILE->buffer, 0, BUFFSIZE); // am zeroizat zona bufferului (y)
        FILE->actualpoz = 0;
        FILE->bufferpoz = 0;
        FILE->bufferlen = 0;
        FILE->hasErr = 0;
        FILE->last_op = 0;
        FILE->eof = 0;
        strcpy(FILE->tip, mode);
        FILE->processHandle = INVALID_HANDLE_VALUE;
        return FILE;
    }
}

int so_fclose(SO_FILE *stream) // This method returns zero if the stream is successfully closed. On failure, EOF is returned.
{
    // sa nu uit - TO DO: dupa ce fac flush sa-l pun si aici
    if (stream != NULL)
    {
        // printf("%d", stream->fd);
        so_fflush(stream);
        int ret_flush = 0;
        if (stream->last_op == 'w')
            ret_flush = so_fflush(stream);
        int isOK = CloseHandle(stream->fh); // If the function succeeds, the return value is nonzero.
                                            // If the function fails, the return value is zero.
                                            // To get extended error information, call GetLastError. - FOarte helpful la debug
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

HANDLE so_fileno(SO_FILE *stream)
// If successful, fileno() returns the file descriptor number associated...
// If unsuccessful, fileno() returns -1 and sets errno
{
    if (stream != NULL && stream->fh != INVALID_HANDLE_VALUE)
        return stream->fh;
    return INVALID_HANDLE_VALUE;
}

int so_feof(SO_FILE *stream) // This function returns a non-zero value when End-of-File indicator
                             // associated with the stream is set, else zero is returned. <=> 1 cand e eof, 0 cand nu
{
    /*if (stream->actualpoz == filesize(stream->fh))
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
    return stream->hasErr; // hasErr;
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

    DWORD cur_poz = SetFilePointer(
        stream->fh,
        offset, // offset 0 
        NULL,   // no 64bytes offset 
        whence);

    if (cur_poz == INVALID_SET_FILE_POINTER)
    {
        stream->hasErr = 1;
        printf("fseek failed");
        return SO_EOF;
    }
    stream->eof = 0;
    stream->actualpoz = cur_poz;
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
    if (stream != NULL)
    {
        if (strcmp(stream->tip, "r") == 0 || stream->last_op != 'w')
        {
            stream->hasErr = 1;
            return SO_EOF; //cazul - nimic de facut
        }
        int fin = stream->bufferlen;
        int start = 0;
        DWORD bytes_scrisi_acm;
        while (start < fin)
        {
            BOOL bRet = WriteFile(
                stream->fh,             /* open file handle */
                stream->buffer + start, /* start of data to write */
                fin - start,            /* number of bytes to write */
                &bytes_scrisi_acm,      /* number of bytes that were written */
                NULL                    /* no overlapped structure */
            );

            if (bRet == FALSE)
            {
                stream->hasErr = 1;
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

    if (strcmp(stream->tip, "w") == 0 || stream->eof == 1)
    {
        printf("Err fgetc, nu se permite operatia");
        stream->hasErr = 1;
        return SO_EOF;
    }
    int char_to_return;
    if (stream->bufferlen == 0 || stream->bufferpoz == stream->bufferlen)
    {
        // stream->bufferpoz = read(stream->fd, stream->buffer, BUFFSIZE);
        int nr_bytes_cititi_acum = -1;
        BOOL dwRet = ReadFile(
            stream->fh,
            stream->buffer,
            BUFFSIZE,
            &nr_bytes_cititi_acum,
            NULL);
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
    if (stream->last_op == 'w')
    {
        printf("CItire la wirte only!");
        stream->hasErr = 1;
        return SO_EOF;
    }
    if (size <= 0 || nmemb <= 0)
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
        // memcpy(ptr + i, &c, 1); //nu-i place windowsului
        ((unsigned char *)ptr)[i] = (unsigned char)c;
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
    if (strcmp(stream->tip, "r") == 0)
    {
        stream->hasErr = 1;
        return SO_EOF;
    }
    if (stream->bufferpoz == BUFFSIZE)
    {
        int nok = so_fflush(stream);
        if (nok < 0)
        {
            stream->hasErr = 1;
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
    if (stream == NULL || strcmp(stream->tip, "r") == 0 || size <= 0 || nmemb <= 0)
    {
        // stream->hasErr = 1;
        return SO_EOF;
    }
    int lim = size * nmemb;
    int i;
    stream->last_op = 'w';
    for (i = 0; i < lim; i++)
    {
        // char c;
        // memcpy(&c, (char *)ptr + i, 1);
        unsigned char c = ((unsigned char *)ptr)[i];
        int ok = so_fputc(c, stream);
        if (ok < 0 && stream->hasErr == 1)
        {
            return SO_EOF;
        }
    }
    return i / size;
}

SO_FILE *so_popen(const char *command, const char *type)
{
    SO_FILE *file = malloc(sizeof(SO_FILE));
    if (file == NULL)
        return NULL;
    file->fh = INVALID_HANDLE_VALUE; // NULL; //??? - cu ce treb sa init?
    strcpy(file->tip, type);
    file->bufferpoz = 0;
    file->actualpoz = 0;
    memset(file->buffer, 0, BUFFSIZE); // am zeroizat zona bufferului (y)
    file->bufferlen = 0;
    file->hasErr = 0;
    file->last_op = 0;
    file->eof = 0;
    strcpy(file->tip, type);
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    BOOL ret;
    ZeroMemory(&sa, sizeof(sa));
    sa.bInheritHandle = TRUE;

    ret = CreatePipe(
        &hReadPipe,
        &hWritePipe,
        &sa,
        0);

    if (ret == FALSE)
    {
        free(file);
        return NULL;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));
    //

    if (type[0] == 'r')
    {
        // am grija ca proc copil sa nu mosteneasca capatul de citire al pipe-ului
        ret = SetHandleInformation(
            hReadPipe,
            HANDLE_FLAG_INHERIT,
            0);
        /* redirect the write pipe to STDOUT */
        si.hStdOutput = hWritePipe;
        // /|\ e echivalentul lui rc = dup2(pipe_fd[child], STDOUT_FILENO);
        /* parent process is going to use STDIN */
        file->fh = hReadPipe;
    }
    else if (type[0] == 'w')
    {
        //aici copilul trb sa nu mosteneasca capatul de scriere
        ret = SetHandleInformation(
            hWritePipe,
            HANDLE_FLAG_INHERIT,
            0);
        si.hStdInput = hReadPipe;
        file->fh = hWritePipe;
    }
    else
    {
        free(file);
        return NULL;
    }

    if (ret == FALSE)
    {
        free(file);
        return NULL;
    }

    char *full_cmd = malloc(strlen(command) + 1 + strlen("cmd /C "));
    if (full_cmd == NULL)
    {
        free(file);
        return NULL;
    }
    strcpy(full_cmd, "cmd /C ");
    strcat(full_cmd, command);

    ret = CreateProcess(
        NULL,     /* No module name (use command line) */
        full_cmd, /* Command line */
        NULL,     /* Process handle is not inheritable */
        NULL,     /* Thread handle not inheritable */
        TRUE,     /* Set handle inheritance to TRUE */
        0,        /* No creation flags */
        NULL,     /* Use parent's environment block */
        NULL,     /* Use parent's starting directory */
        &si,      /* Pointer to STARTUPINFO structure */
        &pi       /* Pointer to PROCESS_INFORMATION structure */
    );

    if (ret == FALSE)
    {
        free(full_cmd);
        free(file);
        return NULL;
    }

    file->processHandle = pi.hProcess;

    /* aici ajunge cu codul doar parintele */
    if (type[0] == 'r')
        CloseHandle(hWritePipe);
    else if (type[0] == 'w')
        CloseHandle(hReadPipe);

    free(full_cmd);
    return file;
}

int so_pclose(SO_FILE *stream)
{
    if (stream != NULL)
    {
        HANDLE pHandle = stream->processHandle;
        int close_status = so_fclose(stream);
        DWORD r = WaitForSingleObject(pHandle, INFINITE);

        if (r == WAIT_FAILED)
            return -1;
        if ((GetExitCodeProcess(pHandle, &r) == TRUE) && (close_status == 0))
            return 0;
    }
    return -1;
}

// int main()
// {
//     int num_WriteFile;
//     int num_ReadFile;
//     HANDLE target_handle;
//     SO_FILE *f;
//     char *tmp;
//     int ret;
//     char *test_work_dir;
//     //int buf_len = 4000;
//     //char buf[4001];
//     // for (int i = 0; i < 4000; i++)
//     //     buf[i] = '*';
//     // buf[4000] = 0;
//     // printf("%s",buf);
//     f = so_fopen("text.txt", "a+");
//     if (!f)
//         printf("Couldn't open file: text.txt\n");
//     num_ReadFile = 0;
//     num_WriteFile = 0;

//     // ret = so_fseek(f, 3000, SEEK_SET);
//     // if (ret != 0)
//     //     printf("Incorrect return value for so_fseek: got %d, expected %d\n", ret, 0);
//     //tmp = malloc(buf_len + 1000);
//     tmp = malloc(100);
//     ret = so_fread(tmp, 1, 10, f);
//     tmp[10]='\0';
//     if (ret != 10)
//         printf("Incorrect return value for so_fread: got %d, expected %d\n", ret, 1000);
//     printf("%s", tmp);
//     //if (memcmp(tmp, &buf[3000], 1000), "Incorrect data\n");

//     ret = so_fseek(f, 0, SEEK_SET);
//     if (ret != 0)
//          printf("Incorrect return value for so_fseek: got %d, expected %d\n", ret, 0);

//     ret = so_fwrite(tmp, 1, 10, f);
//      if (ret != 10)
//          printf("Incorrect return value for so_fwrite: got %d, expected %d\n", ret, 1000);

//     ret = so_fclose(f);
//     if (ret != 0)
//         printf("Incorrect return value for so_fclose: got %d, expected %d\n", ret, 0);

//     return 0;
// }

// int main()
// {
//     SO_FILE *fp;
//     // fp = fopen("file.txt", "a+");
//     fp = so_popen("dir", "r");
//     char cuv[1001];
//     so_fread(cuv, 1, 1000, fp);
//     printf("*%s*", cuv);
//     int status = so_pclose(fp);
//     if (status == -1)
//     {
//         printf("Error reported by pclose()");
//     }
//     return 0;
// }

// int main()
// {
//     SO_FILE *myf = so_fopen("text.txt", "a+");
//     // printf("eof=%d\n", so_feof(myf));
//     // printf("fileno=%d\n", so_fileno(myf));
//     // strcpy(myf->buffer,"naspa");
//     // myf->bufferpoz=strlen("naspa");
//     // char c = so_fgetc(myf);
//     // so_fputc('$', myf);
//     char cuv[20];
//     int size = so_fread(cuv, 1, 100, myf);
//     // printf("Am citit cu fread: %s\n", cuv);
//     so_fwrite(cuv, 1, strlen(cuv), myf);
//     // printf("\nAm extras caracterul %c\nBuffer: %s",c, myf->buffer);
//     // so_fflush(myf);
//     // printf("Am ramas in buff pe poz: %d\n", myf->bufferpoz);
//     so_fclose(myf);

//     /*Exemplu de main luat de pe net si pun so la toate fctiile: //A MEEEEERS*/
//     // SO_FILE *fp;
//     // fp = so_fopen("file.txt", "w+");
//     //  char string[]="This is tutorialspoint.com";
//     //  so_fwrite(string,1,strlen(string), fp);
//     //  /* for(int i=0;i<strlen(string);i++)
//     //  {
//     //      so_fputc(string[i], fp);
//     //  } */

//     // so_fseek(fp, 7, SEEK_SET);
//     // strcpy(string," C Programming Language");
//     // so_fwrite(string,1,strlen(string), fp);
//     // /*
//     // for(int i=0;i<strlen(string);i++)
//     // {
//     //     so_fputc(string[i], fp);
//     // } */
//     // so_fclose(fp);

//     return 0;
// }
/*
int main()
{
    // merge ok si asta
    FILE *fp;
    int c;

    fp = fopen("file.txt", "r");
    while (1)
    {
        c = fgetc(fp);
        if (feof(fp))
        {
            break;
        }
        printf("%c", c);
    }
    fclose(fp);
    return (0);
}
 */