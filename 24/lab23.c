#include<pthread.h>
#include<stdio.h>
#include <semaphore.h>
#include<stdlib.h>
#include <string.h>
#include <signal.h>
#include <alloca.h>
#include <unistd.h>
//#define PRODUSERS_NUM 2
//#define CONSUMERS_NUM 2

pthread_mutex_t mutex;
pthread_cond_t cond;

struct Message {
	char message[81];
	struct Message* next;
	struct Message* prev;
};

struct Queue {
	struct Message* head;
	struct Message* tail;
	int is_droped;
	int mes_count; 
};

void mymsginit(struct Queue* queue) {
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	queue->head = NULL;
	queue->tail = NULL;
	queue->is_droped = 0;
	queue->mes_count = 0;
}

void mymsqdrop(struct Queue* queue) {
	queue->is_droped = 1;
	pthread_cond_broadcast(&cond);
}

void mymsgdestroy(struct Queue* queue) {
	struct Message* tmp = queue->head;
	pthread_mutex_lock(&mutex);
	struct Message* del_tmp;;
	while(tmp) {
		del_tmp  = tmp;
		tmp = tmp->next;
		free(del_tmp);
	}
	pthread_mutex_unlock(&mutex);
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
}

int mymsgget(struct Queue* queue, char* buf, size_t bufsize) {
	if(queue == NULL)
                return 0;
	pthread_mutex_lock(&mutex);
	if(queue->is_droped == 1) {
                return 0;
        }
	while(queue->mes_count == 0 && queue->is_droped != 1)
		pthread_cond_wait(&cond, &mutex);

	if(queue->is_droped == 1) {
		pthread_mutex_unlock(&mutex);
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

//	if(queue->head != NULL)
//	printf("extracted : %s, new head is %s\n", buf, queue->head->message);
//	else { printf("extracted : %s, new head is null\n", buf);}

	if(queue->mes_count == 10)
		pthread_cond_signal(&cond);
	(queue->mes_count)--;
	pthread_mutex_unlock(&mutex);
	return 1;
}

int mymsgput(struct Queue* queue, char* msg) {
	if(queue == NULL)
		return 0;


	pthread_mutex_lock(&mutex);
	if(queue->is_droped == 1) {
                return 0;
        }

	while(queue->mes_count >= 10 && queue->is_droped != 1)
		pthread_cond_wait(&cond, &mutex);

	if(queue->is_droped == 1) {
		pthread_mutex_unlock(&mutex);
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

	if(tmp == NULL) {
		queue->tail = new_mes;
	}

//	printf("added str %s = %s\n", new_mes, queue->head->message);
	if(queue->mes_count == 0)
                pthread_cond_signal(&cond);
	(queue->mes_count)++;
	pthread_mutex_unlock(&mutex);
	return 1;
}

//--------------------

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
