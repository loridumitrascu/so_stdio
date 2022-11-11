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

- fopen;
- fclose;
- fseek;
- ftell;
- fgetc;
- fread;
- fputc;
- fwrite;
- fflush;
- fileno;
- ferror;
- feof.

For this implementations i needed a custom struct to manage the handles/file descriptors and the buffer data(bufferpoz, bufferlen and the actual buffer as a char vector). The buffer supposes some extra care when juggling between different operations. The buffer is the middle-man in-between all interactions with the library. Everything goes through the buffer first, weather it's a read or a write operation so i used a char to remember the last operation applied on the buffer to have no further confusion.

For compiling, good for both Linux and Windows:
```
make build
```
