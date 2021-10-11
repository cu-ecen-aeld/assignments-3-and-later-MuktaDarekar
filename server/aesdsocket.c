// Tester code for assignment 5 part 1
// File:	aesdsocket.c
// Author:	Mukta Darekar
// Reference: https://docs.oracle.com/cd/E19620-01/805-4041/sockets-47146/index.html
//			  https://github.com/stockrt/queue.h/blob/master/sample.c
//			  https://github.com/cu-ecen-aeld/aesd-lectures/blob/master/lecture9/timer_thread.c

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
#include <sys/queue.h>
#include <pthread.h>
#include <sys/time.h>

// macro definition 
#define TOTAL_MSG	3	
#define ERROR		1
#define SUCCESS		0
#define MYPORT		9000
#define DEF_FILEPATH	"/var/tmp/aesdsocketdata"
#define BUFFER_LEN	200

typedef struct
{
    pthread_t thread;
    int fd;
    int acceptedfd;
	struct in_addr sin_addr;
    bool thread_complete_success;
    bool complete_status_flag;

}thread_data;


typedef struct slist_data_s slist_data_t;
struct slist_data_s{

    thread_data params;
    SLIST_ENTRY(slist_data_s) entries;
};

int sockfd;
int exit_on_signal=0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct 
{
    int fd;
    bool timer_thread_success;

}timer_thread_data;


// A thread which runs every 10sec of interval
// Assumes timer_create has configured for sigval.sival_ptr to point to the
// thread data used for the timer
static void timer_thread(union sigval sigval)
{

    timer_thread_data* td = (timer_thread_data*) sigval.sival_ptr;
    
    td->timer_thread_success = true;

    char tmrbuffer[BUFFER_LEN];
    time_t rtime;
    time(&rtime);
    struct tm *timestamp = localtime(&rtime);
    size_t nbytes;

    if((nbytes = strftime(tmrbuffer, BUFFER_LEN, "timestamp:%a, %d %b %Y %T %z\n", timestamp)) == 0)
    {
        syslog(LOG_ERR,"timestamp to string failed");
        td->timer_thread_success = false;
    }

    if(pthread_mutex_lock(&mutex) != 0)
	{
		syslog(LOG_ERR, "pthread_mutex_lock failed");
		td->timer_thread_success = false;
	}

    if(write(td->fd, tmrbuffer, nbytes) == -1)
    {
        syslog(LOG_ERR,"timestamp write failed");
        td->timer_thread_success = false;
    }

    if(pthread_mutex_unlock(&mutex) != 0)
	{
		syslog(LOG_ERR, "pthread_mutex_unlock failed");
		td->timer_thread_success = false;
	}
	
    //pthread_exit(NULL);
}

//Function:	static void signal_handler(int signo)
//Inputs:	signo - Signal number
static void signal_handler(int signo)
{

	syslog(LOG_DEBUG, "in handler\n");
	if(signo == SIGINT || signo==SIGTERM) 
	{
		if(signo == SIGINT)
			syslog(LOG_DEBUG, "Caught signal SIGINT, exiting\n");
		else
			syslog(LOG_DEBUG, "Caught signal SIGTERM, exiting\n");
			
		shutdown(sockfd, SHUT_RDWR);

		exit_on_signal = true;
	}
	
}

void* packetRWthread(void* thread_param)
{
    thread_data *thread_func_args = (thread_data*)thread_param;
	bool status = true;

    int req_size=0;
    int size = BUFFER_LEN;    
    char *rebuffer = NULL;
    char *buffer = (char*)malloc(sizeof(char)*BUFFER_LEN);
	int nbytes = 0;
	ssize_t nr = 0;

	sigset_t mask;
    //Create signal set
    if (sigemptyset(&mask) == -1) 
	{
        syslog(LOG_ERR, "creating empty signal set failed");
		status = false;
    }
	//Add signal SIGINT into created empty set
    if (sigaddset(&mask, SIGINT) == -1) 
	{
        syslog(LOG_ERR, "Adding SIGINT failed");
		status = false;
    }
	//Add signal SIGTERM into created empty set
    if (sigaddset(&mask, SIGTERM) == -1) 
	{
        syslog(LOG_ERR, "Adding SIGTERM failed");
		status = false;
    } 
       
    
    while(1)
    {
    	nbytes = recv(thread_func_args->acceptedfd, buffer+req_size, BUFFER_LEN, 0);
    	if(nbytes == -1)
    	{
		    syslog(LOG_ERR,"receive failed");
			status = false;
			break;
        }
        if (nbytes ==0)
        	break;
        
        req_size = req_size + nbytes;
        	
        if(size < (req_size+1))
        {
        	size += BUFFER_LEN;
        	rebuffer = realloc(buffer,sizeof(char)*size);
        	if(rebuffer == NULL)
        	{
		        syslog(LOG_ERR,"realloc failed");
				status = false;
				break;
		    }
		    buffer = rebuffer;
        }
       	
       	if(strchr(buffer,'\n') != NULL) 
       		break;    	
    }
    
    if(pthread_mutex_lock(&mutex) != 0)
	{
		syslog(LOG_ERR, "pthread_mutex_lock failed");
		status = false;
	}
    // Block signals to avoid partial write
    if (sigprocmask(SIG_BLOCK,&mask,NULL) == -1)
    {
        syslog(LOG_ERR,"sigprocmask failed");
		status = false;
    }	
	nr = write(thread_func_args->fd, buffer, req_size);
	if (nr == -1)	
	{//if error
		syslog(LOG_ERR, "can't write received string in file '%s'", DEF_FILEPATH);
		status = false;
	}
    // Block signals to avoid partial write
    if (sigprocmask(SIG_UNBLOCK,&mask,NULL) == -1)
    {
        syslog(LOG_ERR,"sigprocmask failed");
		status = false;
    }		
	if(pthread_mutex_unlock(&mutex) != 0)
	{
		syslog(LOG_ERR, "pthread_mutex_unlock failed");
		status = false;
	}
    

    lseek(thread_func_args->fd, 0, SEEK_SET);
    
    req_size = 0;
	int ptr=0;
    
    while(1)
    {
     
		if(pthread_mutex_lock(&mutex) != 0)
		{
			syslog(LOG_ERR, "pthread_mutex_lock failed");
			status = false;
		}
		if (sigprocmask(SIG_BLOCK,&mask,NULL) == -1)
		{
		    syslog(LOG_ERR,"sigprocmask failed");
			status = false;
		}
    
    	nr = read(thread_func_args->fd, &buffer[ptr], 1);    
    
		if (sigprocmask(SIG_UNBLOCK,&mask,NULL) == -1)
		{
		    syslog(LOG_ERR,"sigprocmask failed");
			status = false;
		}
		
		if(pthread_mutex_unlock(&mutex) != 0)
		{
			syslog(LOG_ERR, "pthread_mutex_unlock failed");
			status = false;
		}    
		
        if (nr == 1)
        {        	  
        	if(buffer[ptr] == '\n')
        	{
        		req_size = (ptr+1);
        		nbytes = send(thread_func_args->acceptedfd, buffer, req_size, 0);
        		if(nbytes != req_size)
        		{
					syslog(LOG_ERR, "send failed");
					break;
        		}
        		ptr=0;
        		memset(buffer, 0, req_size);
        	}
        	else
        	{
        		ptr++;
        	
				if(size < (ptr+1))
				{
					size += BUFFER_LEN;
					rebuffer = realloc(buffer,sizeof(char)*size);
					if(rebuffer == NULL)
					{
						syslog(LOG_ERR,"realloc failed");
						status = false;
						break;
					}
					buffer = rebuffer;
				}
        	}
        	
        }
        else if (nr == 0)
        {
        	syslog(LOG_DEBUG, "read done");
        	break;
        }
        else
        {
        	syslog(LOG_ERR, "read failed");
			status = false;
			break;
        }
    	
    }
    
    if (status == true)
    {
		syslog(LOG_DEBUG, "Successful");
    }
    
	syslog(LOG_DEBUG, "Closing connection from '%s'\n", inet_ntoa((struct in_addr)thread_func_args->sin_addr));
    close(thread_func_args->acceptedfd);

    //status true
	thread_func_args->thread_complete_success = status;
    thread_func_args->complete_status_flag = true;

    free(buffer);
    free(rebuffer);
    
    //pthread_exit(NULL);
	return NULL;
}


int main(int argc, char* argv[])
{

    openlog(NULL, LOG_CONS, LOG_USER);

	struct sockaddr_in saddr;
	bool exit_on_error = false;
	int opt=1;
	int acceptedfd;
	int fd; 
    socklen_t len;
    int ret = 0;

	slist_data_t *datap = NULL;
	SLIST_HEAD(slisthead,slist_data_s) head;
    SLIST_INIT(&head);
            
	syslog(LOG_INFO, "aesdsocket code started\n");
	
	// check if deamon needs to be started
    if ((argc == 2) && (strcmp("-d", argv[1])==0)) 
	{
		// start deamon
        pid_t pid = fork();
        if (pid == -1) 
		{
            syslog(LOG_ERR, "failed to fork");
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
			exit(EXIT_FAILURE);
		}
		umask(0);
		syslog(LOG_INFO, "SID: %d\n", sid);

		if (chdir("/") == -1) 
		{
			syslog(LOG_ERR, "failed to change dir");
			exit(EXIT_FAILURE);
		}
		syslog(LOG_INFO, "chdir successful\n");

		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		
		syslog(LOG_INFO, "daemon created\n");
    }
    
	// Set signal handler for SIGINT
	if(signal(SIGINT, signal_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "Cannot handle SIGINT!\n");
		exit_on_error = true;
		goto EXITING;
	}

	// Set signal handler for SIGTERM
	if(signal(SIGTERM, signal_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "Cannot handle SIGTERM!\n");
		exit_on_error = true;
		goto EXITING;
	}
    
	//create socket
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		syslog(LOG_ERR, "socket creation failed\n");
		exit_on_error = true;
		goto EXITING;
	}
	syslog(LOG_INFO, "socket created\n");
	
	//reuse socket
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1)
	{
		syslog(LOG_ERR, "setsocketopt failed\n");
		exit_on_error = true;
		goto EXITING;
	}
	syslog(LOG_INFO, "socket created\n");
	saddr.sin_family = PF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(MYPORT);

	//bind socket at port number 9000
	ret = bind(sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in));
	if (ret == -1)
	{
		syslog(LOG_ERR, "socket binding failed\n");
		exit_on_error = true;
		goto EXITING;
	}
	syslog(LOG_INFO, "bind successful\n");
    
    //start listening at port 9000
	ret = listen(sockfd, 10);
	if (ret == -1)
	{
		syslog(LOG_ERR, "socket listening failed\n");
		exit_on_error = true;
		goto EXITING;
	}
	syslog(LOG_INFO, "listening\n");

	//create or open file to store received packets
	fd = open(DEF_FILEPATH, O_CREAT | O_RDWR | O_APPEND | O_TRUNC, 0764);
	if (fd == -1)	
	{//if error
		syslog(LOG_ERR, "can't open or create file '%s'\n", DEF_FILEPATH);
		exit_on_error = true;
		goto EXITING;
	}
	
	
    timer_t timerid;
	struct sigevent *sev = malloc(sizeof(struct sigevent));
    timer_thread_data td;
    int clock_id = CLOCK_MONOTONIC;
    
    memset(sev,0,sizeof(struct sigevent));
    memset(&td,0,sizeof(timer_thread_data));
    td.fd = fd;
    td.timer_thread_success = true;

    //Setup a call to timer_thread passing in the td structure as the sigev_value argument
    sev->sigev_notify = SIGEV_THREAD;
    sev->sigev_value.sival_ptr = &td;
    sev->sigev_notify_function = timer_thread;
    
    if (timer_create(clock_id, sev, &timerid) != 0 ) 
    {
		syslog(LOG_ERR, "can't create timer_thread");
		exit_on_error = true;
		goto EXITING;
    }
    else
    {
    	struct timespec timer_val;
    	ret = clock_gettime(clock_id, &timer_val);
    	if(ret == -1)
        {
			syslog(LOG_ERR, "clock_gettime failed");
			exit_on_error = true;
			goto EXITING;
    	}
    	struct itimerspec interval;
		interval.it_interval.tv_sec = 10;
		interval.it_interval.tv_nsec = 0;

		interval.it_value.tv_sec = timer_val.tv_sec + interval.it_interval.tv_sec;
    	interval.it_value.tv_nsec = timer_val.tv_nsec + interval.it_interval.tv_nsec;
    	if( interval.it_value.tv_nsec > 1000000000L ) 
    	{
		    interval.it_value.tv_nsec -= 1000000000L;
		    interval.it_value.tv_sec += 1;
    	}

		if(timer_settime(timerid, TIMER_ABSTIME, &interval, NULL ) != 0 ) 
		{		   
			syslog(LOG_ERR, "timer_settime failed");
			exit_on_error = true;
			goto EXITING;
		} 
    }
    
    while(exit_on_signal==0) 
    {

		len = sizeof(struct sockaddr);
		
		//accept connection
		acceptedfd = accept(sockfd, (struct sockaddr *) &saddr, &len);
		
		if(exit_on_signal || exit_on_error)
			break;
			
		if (acceptedfd == -1)
		{
			syslog(LOG_ERR, "socket accepting failed\n");
			exit_on_error = true;
			goto EXITING;
		}
		if(exit_on_signal || exit_on_error)
			break;
			
		syslog(LOG_DEBUG, "Accepted connection from '%s'\n", inet_ntoa((struct in_addr)saddr.sin_addr));

		datap = malloc(sizeof(slist_data_t));
		if (datap == NULL)
		{
			syslog(LOG_ERR, "malloc failed\n");
			exit_on_error = true;
			goto EXITING;
		}
			
		SLIST_INSERT_HEAD(&head,datap,entries);
		datap->params.acceptedfd = acceptedfd;
		datap->params.complete_status_flag = false;
		datap->params.thread_complete_success = false;
		datap->params.sin_addr = saddr.sin_addr;
		datap->params.fd=fd;

		pthread_create(&(datap->params.thread), NULL, &packetRWthread, (void*)&(datap->params));

		SLIST_FOREACH(datap,&head,entries)
		{
		    if (datap->params.complete_status_flag == true)
		        pthread_join(datap->params.thread,NULL);
		    else
		    	continue;
    	}
	}

EXITING:

	if (exit_on_error && exit_on_signal==false)
		syslog(LOG_DEBUG, "exiting on failure\n");
	else
		syslog(LOG_DEBUG, "exiting on signal\n");

	//if(acceptedfd)
	//	close(acceptedfd);

	if(sockfd)
		close(sockfd);

	if(fd)	
	{
		close(fd);
		remove(DEF_FILEPATH);
	}
	
	pthread_mutex_destroy(&mutex);
	
	SLIST_FOREACH(datap,&head,entries)
	{
	    if (datap->params.complete_status_flag == false)
	    	pthread_cancel(datap->params.thread);
	}

	while (!SLIST_EMPTY(&head)) 
	{
        datap = SLIST_FIRST(&head);
        SLIST_REMOVE_HEAD(&head, entries);
        free(datap);
		datap = NULL;
    }

	if(td.timer_thread_success == true)
	{
		syslog(LOG_DEBUG,"deleting timerid");
		if (timer_delete(timerid) != 0) 
		{
		    syslog(LOG_ERR,"Error in deleting timerid");
		    exit_on_error = true;
		}
	}
    free(sev);
	closelog();
	
	if (exit_on_error && exit_on_signal==false)
		return 1;
	else
		return 0;
}



