#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    bool status = true;

    int ret = usleep((thread_func_args->wait_obtain_ms)*1000);
    if (ret != 0)
    {
        ERROR_LOG("usleep failed");
        status = false;
    }

    ret = pthread_mutex_lock(thread_func_args->lock);
    if (ret != 0)
    {
        ERROR_LOG("pthread_mutex_lock failed");
        status = false;
    }

    ret = usleep((thread_func_args->wait_release_ms)*1000);
    if (ret != 0)
    {
        ERROR_LOG("usleep failed");
        status = false;
    }
    
    ret = pthread_mutex_unlock(thread_func_args->lock);
    if (ret != 0)
    {
        ERROR_LOG("pthread_mutex_unlock failed");
        status = false;
    }
    
    if (status == true)
    {
		DEBUG_LOG("Successful");
    }
    
	thread_func_args->thread_complete_success = status;
	return(thread_param);
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */
     
    struct thread_data *params = malloc(sizeof(struct thread_data));

    params->thread = thread;
    params->wait_obtain_ms = wait_to_obtain_ms;
    params->wait_release_ms = wait_to_release_ms;
 	params->lock = mutex;
    params->thread_complete_success = false;

    int ret = pthread_create(thread, NULL, &threadfunc, (void *)params);
    if (ret != 0)
    {
    	free(params);
        ERROR_LOG("pthread_create failed");
    	return false;
    }
    else
    {
        DEBUG_LOG("Successful creation of thread");
    	return true;
    }
}



