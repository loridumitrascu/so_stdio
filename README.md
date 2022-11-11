*Dumitrascu Andreea-Loredana - C113D*

## Operating Systems
# Project 1. stdio library
```
___  ___                       _      _ _         _ _ _     
|  \/  |                      | |    | (_)       | (_) |       
| .  . |_   _   ___  ___   ___| |_ __| |_  ___   | |_| |__  ___
| |\/| | | | | / __|/ _ \ / __| __/ _` | |/ _ \  | | | '_ \/ __|
| |  | | |_| | \__ \ (_) |\__ \ || (_| | | (_) | | | | |_) \__ \
\_|  |_/\__, | |___/\___/ |___/\__\__,_|_|\___/  |_|_|_.__/|___/
         __/ |        ______
				 |___/        |______|                                   
```

This project represents my personal approach to stdio library for Linux and Win32 systems. I managed to implement the core functions of the library, such as:


- fopen: initialize and allocate an so_file instance for further functionalities + checking for errors such as incorrect parameters;
- fclose: free the stream and if the last operation was writing, flush the buffer once again;
- fseek: refresh the buffer and change the position of our actualpoz cursor;
- ftell: returns the actualpoz cursor value;
- fgetc: fill the buffer if it's empty (bufflen==0) or if all buffer contents existing in buffer were used (buffpoz=bufflen) and then return the first element unused (buffer[stream->bufferpoz])
- fread: uses a fgetc in a loop to put piece by piece in ptr until error or until maximum size was reached (i=size*nmemb);
- fputc: calls a flush if the buffer is full then adds to the end of the buffer a specified character;
- fwrite: uses a fputc in a loop until error or until maximum size was reached (i=size*nmemb);
- fflush: writes all the contents of the buffer to the file and reinitialize the buffer;
- fileno: returns the file descriptor number or -1 and sets hasErr to 1 if the stream is invalid;
- ferror: returns non-zero value if the hasErr indicator was modified else returns 0;
- feof: returns 0 if we didn't reach the end of file else return non-zero value;
- popen: initialize an SO_FILE instance, sets the pipes accordingly for the parent and child processes, creates the child process to which it loads the command;
- pclose: waits for the child process to end and verify if any error occured.

For this implementations i needed a custom struct to manage the handles/file descriptors and the buffer data(bufferpoz, bufferlen and the actual buffer as a char vector). The buffer supposes some extra care when juggling between different operations. The buffer is the middle-man in-between all interactions with the library. Everything goes through the buffer first, weather it's a read or a write operation so i used a char to remember the last operation applied on the buffer to have no further confusion.

For compiling, good for both Linux and Windows:
```
make build
```
