#include<stdio.h>
#include<pthread.h>

void* execute_thread(void* args) {
	while(1) 
		printf("thread - 1 work\n");
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
