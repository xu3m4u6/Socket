#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/wait.h>

//UDP: 30718

//construct a list of “graphs” 
void read_file() 
{
    /* 2D array declaration*/
    int connections[100][100];

    int bufferLength = 255;
    char line[bufferLength];
    unsigned int line_count = 0;
    char *path = "data1.txt";
    
    /* Open file */
    FILE *file = fopen(path, "r");
    
    if (!file)
    {
        perror(path);
        return EXIT_FAILURE;
    }
    
    /* Get each line until there are none left */
    while (fgets(line, bufferLength, file))
    {
        /* Add a trailing newline to lines that don't already have one */
        if (line[strlen(line) - 1] != '\n')
            printf("\n");
    }
    
    /* Close file */
    if (fclose(file))
    {
        return EXIT_FAILURE;
        perror(path);
    }
}

int main(int argc, char *argv[]) 
{
    read_file();
    start_server();
    listen_to_main();
}
