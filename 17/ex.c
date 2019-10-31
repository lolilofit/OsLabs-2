#include<stdio.h>
#include<pthread.h>
#include <semaphore.h>

sem_t s1;
sem_t s2;
sem_t s3;

void* sort(void* arg) {
	int i;
	for(i = 0; i < 4; i++) {
		sem_wait(&s3);
		printf("sort\n");
		sem_post(&s1);
	}
}


void* add(void* arg) {
        int i;
	for(i = 0; i < 4; i++) {
                sem_wait(&s2);
                printf("add\n");
                sem_post(&s1);
        }
}


void* _print(void* arg) {
	int i; 
	for(i = 0; i < 4; i++) {
                sem_wait(&s1);
		sem_wait(&s1);
                printf("print\n");
                sem_post(&s2);
		sem_post(&s3);
        }
}

int main() {
	sem_init(&s1, 0, 0);
        sem_init(&s2, 0, 1);
	 sem_init(&s3, 0, 1);
	pthread_t thread, thread1, thread2;

        pthread_create(&thread, NULL, add, NULL);
	pthread_create(&thread1, NULL, sort, NULL);
	pthread_create(&thread2, NULL, _print, NULL);
	pthread_exit(0);
}
