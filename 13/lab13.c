#include<stdio.h>
#include<pthread.h>
#include <semaphore.h>
#include<string.h>

sem_t first;
sem_t second;

void* do_work(void* arg) {
	int i;
	for(i = 0; i < 10; i++) {
		sem_wait(&second);
		printf("thread-1 : %d\n", i);
		sem_post(&first);
	}
}


int main(int argc, char* argv[]) {
	int i, status;
	pthread_t thread;

	sem_init(&first, 0, 1);
	sem_init(&second, 0, 0);
	status = pthread_create(&thread, NULL, do_work, NULL);
        if(status != 0) {
                fprintf(stderr, "Error creating thread\n");
        }

	for(i = 0; i < 10; i++) {
		sem_wait(&first);
		printf("thread-0 : %d\n", i);
		sem_post(&second);
	}
	pthread_join(thread, NULL);
	sem_destroy(&first);
	sem_destroy(&second);
	return 0;
}
