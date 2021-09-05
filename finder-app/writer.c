// Tester code for assignment 2 point 3)
// File:	writer.c
// Author:	Mukta Darekar


// Pre-processor directives
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <unistd.h>

#define TOTAL_ARG	3	//including ./writer + filename + string = 3 arguments


int main(int argc, char *argv[])
{

	openlog(NULL, LOG_CONS, LOG_USER);
	
	// verify arguments
	if(argc != 3)
	{
		syslog(LOG_ERR, "Invalid number of arguments, need Filepath>Filename and string to write\n");
		 return 1;
	}
			
	int fd = open(argv[1], O_CREAT | O_RDWR | O_TRUNC, 0664);
	if (fd == -1)
	{
		syslog(LOG_ERR, "can't open or create file %s\n", argv[1]);
		return 1;
	}
	
	ssize_t nr = write(fd, argv[2], strlen(argv[2]));
	if (nr == -1)
	{
		syslog(LOG_ERR, "can't write string '%s' in file '%s'\n", argv[2], argv[1]);
		return 1;		
	}
	else if (nr == strlen(argv[2]))
	{
		syslog(LOG_DEBUG, "Writing %s to file %s\n", argv[2], argv[1]);
	}
	
	closelog();
	return 0;
}



