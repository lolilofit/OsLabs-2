#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include<pthread.h>
#include <stdio.h>
#include <semaphore.h>


sem_t sem_a, sem_b, sem_c, sem_mod;

void* make_a(void* arg) {
	while(1) {
                sleep(1);
                sem_post(&sem_a);
                printf("detail A created\n");
        }

}
void* make_b(void* arg) {
	while(1) {
		sleep(2);
		sem_post(&sem_b);
		printf("detail B created\n");
	}
}
void* make_c(void* arg) {
	while(1) {
                sleep(3);
                sem_post(&sem_c);
                printf("detail C created\n");
        }
}
void* make_mod(void* arg) {
	while(1) {
                sem_wait(&sem_a);
		sem_wait(&sem_b);
                sem_post(&sem_mod);
                printf("module(ab) created\n");
        }
}


int main(int argc, char* argv[]) {
	int count = 0;
	pthread_t t1;    
    	pthread_t t2;
    	pthread_t t3;
    	pthread_t t4;

	sem_init(&sem_a, 0, 0);
	sem_init(&sem_b, 0, 0);
	sem_init(&sem_c, 0, 0);
	sem_init(&sem_mod, 0, 0);

	pthread_create( &t1, NULL, make_a, NULL);
    	pthread_create( &t2, NULL, make_b, NULL);
   	pthread_create( &t3, NULL, make_c, NULL);
        pthread_create( &t4, NULL, make_mod, NULL);

	while(1) {
		sem_wait(&sem_mod);
		sem_wait(&sem_c);
		count++;
		printf("Do vidget with number=%d\n", count);
	}

	return 0;
}
