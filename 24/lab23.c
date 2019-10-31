#include<pthread.h>
#include<stdio.h>
#include <semaphore.h>
#include<stdlib.h>
#include <string.h>

#define PRODUSERS_NUM 2
#define CONSUMERS_NUM 2

pthread_mutex_t  mutex;
pthread_cond_t cound;

struct Message {
	char message[81];
	struct Message* next;
	struct Message* prev;
};

struct Queue {
	struct Message* head;
	struct Message* tail;
	int is_droped;
	int mes_num; 
};

void mymsginit(struct Queue* queue) {
	pthread_mutex_init(&mutex, NULL);
	pthread_coud_init(&cound, NULL);
	queue->head = NULL;
	queue->tail = NULL;
	queue->is_droped = 0;
	queue->mes_num = 0;
}

void mymsqdrop(struct Queue* queue) {
	queue->is_droped = 1;
	pthread_cond_broadcast(&cound);
	//sem_wait(&global);
	//sem_post(&is_empty);
	//sem_post(&is_full);
	//sem_post(&global);
}

void mymsgdestroy(struct Queue* queue) {
	//sem_wait(&global);
	struct Message* tmp = queue->head;
	pthread_mutex_lock(&mutex);
	struct Message* del_tmp;;
	while(tmp) {
		del_tmp  = tmp;
		tmp = tmp->next;
		free(del_tmp);
	}
	pthread_mutex_unlock(&mutex);
	pthread_cond_destroy(&cound);
	pthread_mutex_destroy(&mutex);
	//sem_post(&global);
	//sem_destroy(&global);
	//sem_destroy(&is_empty);
	//sem_destroy(&is_full);
}

int mymsgget(struct Queue* queue, char* buf, size_t bufsize) {
	if(queue == NULL)
                return 0;

	//sem_wait(&global);
	 if(queue->is_droped == 1) {
                //sem_post(&global);
                return 0;
        }
	pthread_mutex_lock(&mutex);
	if(queue->mes_count == 0)
		pthread_coud_wait(&coud, &mutex);

	//sem_wait(&is_full);
	if(queue->is_droped == 1) {
		pthread_mutex_unlock(&mutex);
 		//sem_post(&is_full);
                //sem_post(&global);
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
	//char mes[81] = "";
	//sprintf(buf, "%s", "");
	strncpy(buf, res->message, bufsize);

	if(queue->head != NULL)
	printf("extracted : %s, new head is %s\n", buf, queue->head->message);
	else { printf("extracted : %s, new head is null\n", buf);}
//	sem_post(&is_empty);
//	sem_post(&global);
	(queue->mes_count)--;
	pthread_mutex_unlock(&mutex);
}

int mymsgput(struct Queue* queue, char* msg) {
	if(queue == NULL)
		return 0;

	//sem_wait(&global);
	if(queue->is_droped == 1) {
		//sem_post(&global);
		return 0;
	}

	pthread_mutex_lock(&mutex);
	//sem_wait(&is_empty);
	if(queue->mes_count > 10)
		pthread_coud_wait(&coud, &mutex);

	if(queue->is_droped == 1) {
		pthread_mutex_unlock(&mutex);
                //sem_post(&is_empty);
		//sem_post(&global);
                return 0;
        }

	printf("create new message\n");
	struct Message* new_mes;
	new_mes = (struct Message*)malloc(sizeof(struct Message));
	new_mes->prev = NULL;
	new_mes->next = NULL;

	struct Message* tmp;
	//copy only 80 symb
	printf("write message\n");
	sprintf(new_mes->message, "%s", "");
	strncat(new_mes->message, msg, 80);

	printf("new head\n");
	tmp = queue->head;
	queue->head = new_mes;
	if(tmp != NULL) {
		printf("not first\n");
		(queue->head)->next = tmp;
		tmp->prev = queue->head;
	}
	//else{queue->head = tmp;}

	printf("add tail\n");
	if(tmp == NULL) {
		queue->tail = new_mes;
	}

	printf("added str %s = %s\n", new_mes, queue->head->message);
	//sem_post(&is_full);
	//sem_post(&global);
	(queue->mes_count)++;
	pthread_mutex_unlock(&mutex);
}


int main(int argc, char* argv[]) {
	struct Queue* q;
	q = (struct Queue*)malloc(sizeof(struct Queue));
	printf("try to init\n");
	mymsginit(q);
	int i;
	char* res;
        res = (char*)malloc(sizeof(char)*20);

	printf("try to put\n");
	mymsgput(q, "mes1");
	mymsgput(q, "mes2");
	printf("try to get\n");
	mymsgget(q, res, 20);

	printf("try to drop\n");
	mymsqdrop(q);
	printf("try to destroy\n");
	mymsgdestroy(q);

	free(q);

	return 0;
}
