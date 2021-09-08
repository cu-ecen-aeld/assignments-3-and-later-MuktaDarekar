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

// Macro directives
#define TOTAL_ARG	3	//including ./writer + filename + string = 3 arguments
#define ERROR		1
#define SUCCESS	0

//Function:	int main(int argc, char *argv[])
//Inputs:	argc - number of arguments, argv[] - arguments fed
//Return:	write operation status 0-success, 1-error
//Brief:	writes given string in given file

int main(int argc, char *argv[])
{
	openlog(NULL, LOG_CONS, LOG_USER);
	int ret = SUCCESS;
	
	// verify arguments 
	if(argc != TOTAL_ARG)
	{
		syslog(LOG_ERR, "Invalid number of arguments, need Filepath>Filename and string to write\n");
		ret = ERROR;
	}
	
	//execute only if no error
	if(ret == SUCCESS)
	{
		//Take argument with filename and path in one variable
		const char *filepath=argv[1];
		
		//Take argument with string to be written in one variable
		const char *writestr=argv[2];
			
		//Open given file, if doesn't exists then create	
		int fd = open(filepath, O_CREAT | O_RDWR | O_TRUNC, 0664);
		if (fd == -1)	
		{//if error
			syslog(LOG_ERR, "can't open or create file %s\n", filepath);
			ret = ERROR;
		}
		
		//execute only if no error
		if(ret == SUCCESS)
		{
			//Write given/created file with given string
			ssize_t nr = write(fd, writestr, strlen(writestr));
			if (nr == -1)	
			{//if error
				syslog(LOG_ERR, "can't write string '%s' in file '%s'\n", writestr, filepath);
				ret = ERROR;		
			}
			else if (nr == strlen(writestr))
			{//if no.of bytes written equals to string len then write successful
				syslog(LOG_DEBUG, "Writing %s to file %s\n", writestr, filepath);
				ret = SUCCESS;
			}
		}
	}
	
	closelog();
	return ret;
}



