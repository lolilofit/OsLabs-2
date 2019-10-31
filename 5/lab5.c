#include<stdio.h>
#include<pthread.h>
#include <sys/types.h>

void cleanup_func(void* args) {
	printf("thread cleanup handler\n");
}


void* execute_thread(void* args) {
	int oldstate;
        if(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate) != 0)
		return 0;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_push(cleanup_func, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        while(1) 
                printf("thread - 1 work\n");
	pthread_cleanup_pop(0); 
        return 0;
}


int main(int argc, char* argv[]) {
        pthread_t thread;
        int status;
        int status_addr;

        status = pthread_create(&thread, NULL, execute_thread, NULL);
        if(status != 0)
                fprintf(stderr, "error creating thread");

        sleep(2);
        pthread_cancel(thread);

        status = pthread_join(thread, (void**)&status_addr);
        if(status != 0)
                fprintf(stderr, "error joining thread");
        return 0;
}

