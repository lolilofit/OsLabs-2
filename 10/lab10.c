#include<stdio.h>
#include<pthread.h>
#include<errno.h>
#include <sched.h>

pthread_mutex_t en;
pthread_mutex_t ex;
pthread_mutex_t it;
pthread_mutex_t add;
int flag = 1;

void* do_work(void *args) {
        int i, res;

	pthread_mutex_lock(&it);
	pthread_mutex_lock(&add);
	flag = 0;
	pthread_mutex_unlock(&add);

	int count = 0;
        //for(i = 0; i < 3; i++) {
	while(count < 10) {
			if(count%3 == 0) {
				if(pthread_mutex_lock(&en) != 0) 
					continue;
	       	 		printf("thread 1 - %d\n", count);
				pthread_mutex_unlock(&it);
			}
			if(count%3 == 1) {
				if(pthread_mutex_lock(&ex) != 0) 
					continue;
				printf("thread 1 - %d\n", count);
				pthread_mutex_unlock(&en);
			}
			if(count%3==2) {
				if(pthread_mutex_lock(&it) != 0) 
					continue;
				sleep(1);
				printf("thread 1 - %d\n", count);
				pthread_mutex_unlock(&ex);
			}
			count++;
		}
	 pthread_mutex_unlock(&it);
        return 0;
}


int main(int argc, char** argv) {
        pthread_t thread;
        int status;
        int status_addr;
	
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutex_init(&en, &attr);
	pthread_mutex_init(&ex, &attr);
	pthread_mutex_init(&it, &attr);
	pthread_mutex_init(&add, &attr);

	pthread_mutex_lock(&en);

	status = pthread_create(&thread, NULL, do_work, NULL);
        if(status != 0) {
                fprintf(stderr, "Error creating thread\n");
        }

//	sleep(1);
	pthread_mutex_lock(&add);
	while(flag) {
		pthread_mutex_unlock(&add); 
		sched_yield();
		pthread_mutex_lock(&add);
	}
	pthread_mutex_unlock(&add);

	int count = 0, res;
//	 for(i = 0; i < 3; i++) {
	while(count < 10) {
			if(count%3 == 0) {
				if(pthread_mutex_lock(&ex) != 0) 
					continue;
                        	printf("thread 0 - %d\n", count);
				pthread_mutex_unlock(&en);
			}
			if(count%3 == 1) {
				if(pthread_mutex_lock(&it) != 0)
					continue;
				printf("thread 0 - %d\n", count);
				pthread_mutex_unlock(&ex);
			}
			if(count%3 == 2) {
				if(pthread_mutex_lock(&en)!= 0)
					continue;
				printf("thread 0 - %d\n", count);
				pthread_mutex_unlock(&it);
			}
			count++;
	}
	 pthread_mutex_unlock(&en);

        status = pthread_join(thread, (void**)&status_addr);
        if(status != 0) {
                fprintf(stderr, "error\n");
        }


	pthread_mutex_destroy(&ex);
	pthread_mutex_destroy(&en);
	pthread_mutex_destroy(&it);
        return 0;
}

