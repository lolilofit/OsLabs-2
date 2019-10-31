#include <stdio.h>
#include <pthread.h>
#include <string.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;

void* do_work(void* arg) {
	int i;

	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&cond);
	for(i = 0; i < 10; i++) {
		pthread_cond_wait(&cond, &mutex);
	//	pthread_cond_signal(&cond);
		printf("%s\n", (char*)arg);
		pthread_cond_signal(&cond);
	}
	pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&cond);
	return NULL;
}


int main() {
	pthread_t thread;
        int status, i;
        int status_addr;
//	pthread_mutexattr_t attr;
//        pthread_mutexattr_init(&attr);
//	pthread_mutex_init(&mutex, &attr);
//	pthread_cond_init(&cond, NULL);

	
	status = pthread_create(&thread, NULL, do_work, "thread-1");
        if(status != 0) {
                fprintf(stderr, "Error creating thread\n");
        }

//	sleep(2);
	pthread_mutex_lock(&mutex);
        for(i = 0; i < 10; i++) {
		pthread_cond_wait(&cond, &mutex);
                printf("%s\n", "thread-0");
                pthread_cond_signal(&cond);
        }
        pthread_mutex_unlock(&mutex);

	status = pthread_join(thread, (void**)&status_addr);
        if(status != 0) {
                fprintf(stderr, "error\n");
        }
	pthread_mutex_destroy(&mutex);
	return 0;
}
