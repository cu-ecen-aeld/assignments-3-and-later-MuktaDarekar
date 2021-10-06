// Tester code for assignment 5 part 1
// File:	writer.c
// Author:	Mukta Darekar
// Reference: https://docs.oracle.com/cd/E19620-01/805-4041/sockets-47146/index.html


// Pre-processor directives
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

// Macro directives
#define TOTAL_MSG	3	
#define ERROR		1
#define SUCCESS		0
#define MYPORT		9000
#define DEF_FILEPATH	"/var/tmp/aesdsocketdata"

int sockfd = 0;
char *sendbuffer = NULL;
int acceptedfd = 0;
int fd = 0;
sigset_t mask;

//filename and path in one variable
const char *filepath = DEF_FILEPATH;

//Function:	static void signal_handler(int signo)
//Inputs:	signo - Signal number

static void closeall(void)
{
	if(sendbuffer != NULL)	
		free(sendbuffer);

	if(acceptedfd)
		close(acceptedfd);

	if(sockfd)
		close(sockfd);

	if(fd)	
	{
		close(fd);
		remove(filepath);
	}
	
	closelog();	
}

//Function:	static void signal_handler(int signo)
//Inputs:	signo - Signal number

static void signal_handler(int signo)
{

	syslog(LOG_DEBUG, "in handler\n");
	
	if(signo == SIGINT)
		syslog(LOG_DEBUG, "Caught signal SIGINT, exiting\n");
	else
		syslog(LOG_DEBUG, "Caught signal SIGTERM, exiting\n");

	closeall();

	exit(EXIT_SUCCESS);
	
}


//Function:	int main(int argc, char *argv[])
//Inputs:	argc - number of arguments, argv[] - arguments fed

int main(int argc, char *argv[])
{
	openlog(NULL, LOG_CONS, LOG_USER);

	struct sockaddr_in saddr;
	char buffer[50] = {0};
	int nbytes = 0;
	ssize_t nr = 0;
	unsigned int total_bytes = 0;
	off_t location =0;
	int deamon=0;

	// check if deamon needs to be started
    if ((argc == 2) && (strcmp("-d", argv[1])==0)) 
		deamon = 1;
		
	syslog(LOG_INFO, "aesdsocket code started\n");

	// Set signal handler for SIGINT
	if(signal(SIGINT, signal_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "Cannot handle SIGINT!\n");
		exit(EXIT_FAILURE);
	}

	// Set signal handler for SIGTERM
	if(signal(SIGTERM, signal_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "Cannot handle SIGTERM!\n");
		exit(EXIT_FAILURE);
	}

	//Create signal set
    if (sigemptyset(&mask) == -1) 
	{
        syslog(LOG_ERR, "creating empty signal set failed");
        exit(EXIT_FAILURE);
    }
	//Add signal SIGINT into created empty set
    if (sigaddset(&mask, SIGINT) == -1) 
	{
        syslog(LOG_ERR, "Adding SIGINT failed");
        exit(EXIT_FAILURE);
    }
	//Add signal SIGTERM into created empty set
    if (sigaddset(&mask, SIGTERM) == -1) 
	{
        syslog(LOG_ERR, "Adding SIGTERM failed");
        exit(EXIT_FAILURE);
    }
	syslog(LOG_INFO, "signal handler set\n");

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		syslog(LOG_ERR, "socket creation failed\n");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "socket created\n");

	saddr.sin_family = PF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(MYPORT);

	int ret = bind(sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in));
	if (ret == -1)
	{
		syslog(LOG_ERR, "socket binding failed\n");
		closeall();
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "bind successful\n");

	// start deamon
	if (deamon) 
	{
        pid_t pid = fork();
        if (pid == -1) 
		{
            syslog(LOG_ERR, "failed to fork");
			closeall();
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
		{
            exit(EXIT_SUCCESS);
        }

		syslog(LOG_INFO, "fork successful\n");
	
        pid_t sid = setsid();		
		if (sid == -1) 
		{
			syslog(LOG_ERR, "failed to setsid");
			closeall();
			exit(EXIT_FAILURE);
		}
		
		syslog(LOG_INFO, "SID: %d\n", sid);

		if (chdir("/") == -1) 
		{
			syslog(LOG_ERR, "failed to change dir");
			closeall();
			exit(EXIT_FAILURE);
		}
		syslog(LOG_INFO, "chdir successful\n");

		//open("/dev/null", O_RDWR);
		//dup(STDIN_FILENO);
		//dup(STDOUT_FILENO);
		//dup(STDERR_FILENO);

		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
    }
	syslog(LOG_INFO, "daemon created\n");
	
	ret = listen(sockfd, 10);
	if (ret == -1)
	{
		syslog(LOG_ERR, "socket listening failed\n");
		closeall();
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "listening\n");
	
	socklen_t len = sizeof(struct sockaddr);

	fd = open(filepath, O_CREAT | O_RDWR | O_APPEND | O_TRUNC, 0764);
	if (fd == -1)	
	{//if error
		syslog(LOG_ERR, "can't open or create file '%s'\n", filepath);
		closeall();
		exit(EXIT_FAILURE);
	}
	syslog(LOG_DEBUG, "opened or created file '%s' successfully\n", filepath);
	
	while(1)
	{
		acceptedfd = accept(sockfd, (struct sockaddr *) &saddr, &len);
		if (acceptedfd == -1)
		{
			syslog(LOG_ERR, "socket accepting failed\n");
			closeall();
			exit(EXIT_FAILURE);
		}

		syslog(LOG_DEBUG, "Accepted connection from '%s'\n", inet_ntoa((struct in_addr)saddr.sin_addr));
	

		//Signals from set are added to invoking process's signal mask
        if (sigprocmask(SIG_BLOCK, &mask, NULL)) 
		{
            syslog(LOG_ERR, "sigprocmask");
			closeall();
            exit(EXIT_FAILURE);
        }

		do
		{
			memset(buffer, 0, sizeof(buffer));
			
			nbytes = recv(acceptedfd, buffer, sizeof(buffer), 0);

			if (nbytes)
			{
				// syslog(LOG_DEBUG, "nbytes - %d\n", nbytes);
				// syslog(LOG_DEBUG, "'%s'\n", buffer);
				if(nbytes > strlen(buffer))
					nr = write(fd, buffer, strlen(buffer));
				else
					nr = write(fd, buffer, nbytes);
					
				if (nr == -1)	
				{//if error
					syslog(LOG_ERR, "can't write received string in file '%s'\n", filepath);
					break;	
				}
				total_bytes += nr;
			}
			else
				break;

		}while(strchr(buffer, '\n') == NULL);
		
				
		nbytes=0;
		lseek(fd, 0, SEEK_SET);
		
		sendbuffer = (char *)malloc(sizeof(char) * (total_bytes+location));
		if (sendbuffer == NULL)
			syslog(LOG_ERR, "malloc failed\n");
		
		memset(sendbuffer, 0, total_bytes+location);
		nr = read(fd, sendbuffer, total_bytes+location);
		if (nr != total_bytes+location)	
		{//if error
			syslog(LOG_ERR, "can't read proper bytes from file '%s'\n", filepath);
			break;	
		}
		
		//syslog(LOG_DEBUG, "'%s'\n", sendbuffer);
		nbytes = send(acceptedfd, sendbuffer, nr, 0);
		if(nbytes != nr)
		{
			syslog(LOG_ERR, "not all bytes sent\n");
			break;
		}
		//syslog(LOG_DEBUG, "nbytes - %d\n", nbytes);
		//total_bytes -= nbytes;
		total_bytes = 0;
		
		location=lseek(fd, 0, SEEK_END);

		syslog(LOG_DEBUG, "Closed connection from '%s'\n", inet_ntoa((struct in_addr)saddr.sin_addr));


		//Signals from set are added to invoking process's signal mask
        if (sigprocmask(SIG_UNBLOCK, &mask, NULL)) 
		{
            syslog(LOG_ERR, "sigprocmask");
			closeall();
            exit(EXIT_FAILURE);
        }
	
		free(sendbuffer);
		close(acceptedfd);
	}

	remove(filepath);
	close(sockfd);
	close(fd);
	
	closelog();
	return 0;
}



