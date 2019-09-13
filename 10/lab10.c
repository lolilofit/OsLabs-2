#include<stdio.h>
#include<pthread.h>
#include<errno.h>

pthread_mutex_t first;
pthread_mutex_t second;


void* do_work(void *args) {
        int i;
	pthread_cond_t _cound;
        pthread_mutex_t _cound_lock;
        pthread_mutex_init(&_cound, &_cound_lock);


	pthread_mutex_lock(&_cound_lock);
        for(i = 0; i < 10; i++) {
			pthread_mutex_lock(&first);
//		 	if(pthread_mutex_lock(&first) == EDEADLK) {
//				 pthread_cond_wait(&_cound, &_cound_lock);
//			}
               	 	printf("thread 1 - %d\n", i);
		 	pthread_mutex_unlock(&second);
        }
	pthread_mutex_unlock(&_cound_lock);
        return 0;
}


int main(int argc, char** argv) {
        pthread_t thread;
        int status;
        int status_addr;


	pthread_cond_t cound;
	pthread_mutex_t cound_lock;
	pthread_mutex_init(&cound, &cound_lock);

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutex_init(&first, &attr);
	pthread_mutex_init(&second, &attr);        
//	pthread_mutex_init(&third, &attr);

	pthread_mutex_lock(&second);
//	pthread_mutex_lock(&third);
	
//	char message_0 = "thread-0";
//	char message_1 = "thread-1";

	status = pthread_create(&thread, NULL, do_work, NULL);
        if(status != 0) {
                fprintf(stderr, "Error creating thread\n");
        }

	int i;
	pthread_mutex_lock(&cound_lock); 
	 for(i = 0; i < 10; i++) {
			pthread_mutex_lock(&second);  
   //                   if(pthread_mutex_lock(&second) == EDEADLK) {
//				pthread_cond_wait(&cound, &cound_lock);
//			}
                        printf("thread 0 - %d\n", i);
                        pthread_mutex_unlock(&first);
        }
	pthread_mutex_unlock(&cound_lock);



        status = pthread_join(thread, (void**)&status_addr);
        if(status != 0) {
                fprintf(stderr, "error\n");
        }


	pthread_mutex_destroy(&first);
	pthread_mutex_destroy(&second);
	pthread_mutex_destroy(&cound_lock);
        return 0;
}

