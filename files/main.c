#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static char ascii_to_hex(
    char    key
    )
{
    switch(key)
    {
        case '0':
            return 0; 
        case '1':
            return 1; 
        case '2':
            return 2; 
        case '3':
            return 3; 
        case '4':
            return 4; 
        case '5':
            return 5; 
        case '6':
            return 6; 
        case '7':
            return 7; 
        case '8':
            return 8; 
        case '9':
            return 9; 
        case 'a':
            return 10; 
        case 'b':
            return 11; 
        case 'c':
            return 12; 
        case 'd':
            return 13; 
        case 'e':
            return 14; 
        case 'f':
            return 15; 
        default:
            printf("%c\n", key); 
            assert(0); 
    }
}

int main()
{
    FILE   *read_ptr; 
    FILE   *write_ptr; 
    char    buffer[2]; 
    unsigned int bytes_read; 
    char    write; 


    read_ptr = fopen("frame.txt", "rb"); 
    write_ptr = fopen("binary.h264", "wb"); 

    while(1)
    {
        bytes_read = fread(buffer, 2, 1, read_ptr); 
        if(bytes_read == 0)
        {
            break; 
        }

        printf("%c %c", buffer[0], buffer[1]); 

        write = (ascii_to_hex(buffer[0]) << 4) | ascii_to_hex(buffer[1]); 

        fwrite(&write, 1, 1, write_ptr); 

        bytes_read = fread(buffer, 1, 1, read_ptr); 
        if(bytes_read == 0)
        {
            break; 
        }
    }

    fclose(write_ptr); 

    return 0; 
}

