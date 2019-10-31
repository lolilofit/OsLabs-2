#include <stdio.h>
#include <pthread.h>
#include <string.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
int flag1 = 0, flag = 0;


void* do_work(void* arg) {
	int i;

//	sleep(2);
	for(i = 0; i < 10; i++) {
		pthread_mutex_lock(&mutex);
		flag=0;

		pthread_cond_wait(&cond, &mutex);
		flag1 = 1;

		printf("%s\n", (char*)arg);

		while(flag == 0) {
			pthread_mutex_unlock(&mutex);
			pthread_cond_signal(&cond1);
			pthread_mutex_lock(&mutex);
		}
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}


int main() {
	pthread_t thread;
        int status, i;
        int status_addr;

	status = pthread_create(&thread, NULL, do_work, "thread-1");
        if(status != 0) {
                fprintf(stderr, "Error creating thread\n");
        }

        for(i = 0; i < 10; i++) {
		flag1 = 0;

		pthread_mutex_lock(&mutex);
		printf("%s\n", "thread-0");

		while(flag1 == 0) {
		pthread_mutex_unlock(&mutex);
		pthread_cond_signal(&cond);
		pthread_mutex_lock(&mutex);
		}

		pthread_cond_wait(&cond1, &mutex);
		flag = 1;
		pthread_mutex_unlock(&mutex);
        }

	status = pthread_join(thread, (void**)&status_addr);
        if(status != 0) {
                fprintf(stderr, "error\n");
        }
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&mutex1);
	return 0;
}
