#include<stdio.h>
#include<pthread.h>
#include <semaphore.h>
#include<string.h>

struct sem_c {
	int val;
	pthread_mutex_t m;
	pthread_cond_t c;
};

int sem_c_init(struct sem_c* sem, int value) {
	int ret = 0;
	if((ret = pthread_mutex_init(&(sem->m), NULL)) < 0)
		return ret;
	if((ret = pthread_cond_init(&(sem->c), NULL)) < 0)
		return ret;
	sem->val = value;
	return 0;
}

int post(struct sem_c* sem) {
	//check
	pthread_mutex_lock(&(sem->m));
	sem->val = sem->val + 1;
	pthread_cond_signal(&(sem->c));
	pthread_mutex_unlock(&(sem->m));
	return 0;
}

int wait(struct sem_c* sem) {
	pthread_mutex_lock(&(sem->m));
	while(sem->val == 0) {
		pthread_cond_wait(&(sem->c), &(sem->m));
	}
	sem->val = sem->val - 1;
	pthread_mutex_unlock(&(sem->m));
	return 0;
}

int destroy(struct sem_c* sem) {
	 pthread_cond_destroy(&(sem->c));
	 pthread_mutex_destroy(&(sem->m));
}

struct sem_c first;
struct sem_c second;

void* do_work(void* arg) {
	int i;
	for(i = 0; i < 10; i++) {
		//sem_wait(&second);
		wait(&second);
		printf("thread-1 : %d\n", i);
		post(&first);
		//sem_post(&first);
	}
}


int main(int argc, char* argv[]) {
	int i, status;
	pthread_t thread;

	sem_c_init(&first, 1);
	sem_c_init(&second, 0);
	status = pthread_create(&thread, NULL, do_work, NULL);
        if(status != 0) {
                fprintf(stderr, "Error creating thread\n");
        }

	for(i = 0; i < 10; i++) {
		//sem_wait(&first);
		wait(&first);
		printf("thread-0 : %d\n", i);
		post(&second);
		//sem_post(&second);
	}
	pthread_join(thread, NULL);
	destroy(&first);
	destroy(&second);
	return 0;
}
