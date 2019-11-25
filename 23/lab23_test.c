#include<pthread.h>
#include<stdio.h>
#include <semaphore.h>
#include<stdlib.h>
#include <string.h>
#include <signal.h>
#include <alloca.h>
#include <unistd.h>

sem_t is_empty;
sem_t is_full;
//sem_t global;
pthread_mutex_t global;

struct Message {
	char message[81];
	struct Message* next;
	struct Message* prev;
};

struct Queue {
	struct Message* head;
	struct Message* tail;
	int is_droped;
};

void mymsginit(struct Queue* queue) {
	sem_init(&is_empty, 0, 10);
	sem_init(&is_full, 0, 0);
	//sem_init(&global, 0, 1);
	pthread_mutex_init(&global, NULL);

	queue->head = NULL;
	queue->tail = NULL;
	queue->is_droped = 0;
}

void mymsqdrop(struct Queue* queue) {
//	sem_wait(&global);
	pthread_mutex_lock(&global);
	queue->is_droped = 1;
	sem_post(&is_empty);
	sem_post(&is_full);
	 pthread_mutex_unlock(&global);
//	sem_post(&global);
}

void mymsgdestroy(struct Queue* queue) {
	//sem_wait(&global);
	 pthread_mutex_lock(&global);
	struct Message* tmp = queue->head;

	struct Message* del_tmp;;
	while(tmp) {
		del_tmp  = tmp;
		tmp = tmp->next;
		free(del_tmp);
	}
	pthread_mutex_unlock(&global);
	//sem_post(&global);
	//sem_destroy(&global);
	pthread_mutex_destroy(&global);
	sem_destroy(&is_empty);
	sem_destroy(&is_full);
}

int mymsgget(struct Queue* queue, char* buf, size_t bufsize) {
	if(queue == NULL)
                return 0;

	sem_wait(&is_full);
	//sem_wait(&global);
	pthread_mutex_lock(&global);

	if(queue->is_droped == 1) {
		//sem_post(&global);
 		pthread_mutex_unlock(&global);
		sem_post(&is_full);
                return 0;
        }
	struct Message* res = queue->tail;
	if(queue->head == queue->tail) {
		queue->head = NULL;
		queue->tail = NULL;
	}
	else {
		queue->tail = queue->tail->prev;
		queue->tail->next = NULL;
	}
	strncpy(buf, res->message, bufsize);

	//if(queue->head != NULL)
	//printf("extracted : %s, new head is %s\n", buf, queue->head->message);
	//else { printf("extracted : %s, new head is null\n", buf);}
	
	//sem_post(&global);
	pthread_mutex_unlock(&global);
	sem_post(&is_empty);
	return 1;
}

int mymsgput(struct Queue* queue, char* msg) {
	if(queue == NULL)
		return 0;

	sem_wait(&is_empty);
	//sem_wait(&global);
	pthread_mutex_lock(&global);

	if(queue->is_droped == 1) {
		//sem_post(&global);
		pthread_mutex_unlock(&global);
		sem_post(&is_empty);
                return 0;
        }

	struct Message* new_mes;
	new_mes = (struct Message*)malloc(sizeof(struct Message));
	new_mes->prev = NULL;
	new_mes->next = NULL;

	struct Message* tmp;
	//copy only 80 symb
	sprintf(new_mes->message, "%s", "");
	strncat(new_mes->message, msg, 80);

	tmp = queue->head;
	queue->head = new_mes;
	if(tmp != NULL) {
		(queue->head)->next = tmp;
		tmp->prev = queue->head;
	}
	//else{queue->head = tmp;}

	if(tmp == NULL) {
		queue->tail = new_mes;
	}

	//sem_post(&global);
	pthread_mutex_unlock(&global);
	sem_post(&is_full);
	return 1;
}

//--------------------------------



void *producer(void *pq) {
	struct Queue *q=(struct Queue*)pq;
	int i=0;

	for(i=0; i<1000; i++) {
		char buf[40];
		sprintf(buf, "Message %d from thread %d", i, pthread_self());
		if (mymsgput(q, buf) == 0) return NULL;
	}

	return NULL;
}

void *consumer(void *pq) {
	struct Queue *q=( struct Queue*)pq;
	int i=0;

	do {
		char buf[41];
		i=mymsgget(q, buf, sizeof(buf));
		if (i==0) {return NULL;}
		else {printf("Received by thread %d: %s\n", pthread_self(), buf);}
	} while(1);
	return NULL;
}

int pleasequit=0;

void siginthandler(int sig) {
	pleasequit=1;
	signal(sig, siginthandler);
}


int main(int argc, char* argv[]) {
	struct Queue q;
	int nproducers, nconsumers;
	pthread_t *producers, *consumers;
	int i;
	
	if (argc<2) {
		fprintf(stderr, "Usage: %s nproducers nconsumers\n", argv[0]);
		return 0;
	}
	nproducers=atol(argv[1]);
	nconsumers=atol(argv[2]);
	if (nproducers==0 || nconsumers==0) {
		fprintf(stderr, "Usage: %s nproducers nconsumers\n", argv[0]);
		return 0;
	}	

	producers=alloca(sizeof(pthread_t)*nproducers);
	consumers=alloca(sizeof(pthread_t)*nconsumers);
	signal(SIGINT, siginthandler);
	mymsginit(&q);
	for(i=0; i<nproducers || i<nconsumers; i++) {
		if (i<nproducers) {
			pthread_create(producers+i, NULL, producer, &q);
		} 
		if (i<nconsumers) {
			pthread_create(consumers+i, NULL, consumer, &q);
		}
	}
	// while (!pleasequit) pause();
        sleep(3);

	mymsqdrop(&q);
	for(i=0; i<nproducers || i<nconsumers; i++) {
		if(i<nproducers) {
			pthread_join(producers[i], NULL);
		}
		if (i<nconsumers) {
			pthread_join(consumers[i], NULL);
		}
	}
	mymsgdestroy(&q);
        printf("All threads quit and queue destroyed\n");
	return 0;	
}
