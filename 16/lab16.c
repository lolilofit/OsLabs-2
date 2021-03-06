#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>

pthread_mutex_t list_mutext;

struct List{
	struct List* next;
	char* str;
};

void free_mem(struct List* list) {
	if(list->next == NULL) {
		free(list->str);
	}
	else {
		free_mem(list->next);
		free(list);
	}
}

struct List* create_head() {
	struct List* head;
	head = (struct List*)malloc(sizeof(struct List));
	head->next = NULL;
	head->str= NULL;
	return head;
}

void add(struct List* list, char* str){
	pthread_mutex_lock(&list_mutext);
	struct List* tmp = list->next;
	struct List* el = (struct List*)malloc(sizeof(struct List));
	el->str = str;
	el->next = tmp;
	list->next=el;
	pthread_mutex_unlock(&list_mutext);
}


void print_all(struct List* list) {

	printf("let's print\n");
	pthread_mutex_lock(&list_mutext);
	if(list != NULL) {
		struct List* cur = list->next;
		printf("\n");
		while(cur) {
			printf("%s\n", cur->str);
			cur = cur->next;
		}
		printf("\n");
	}
 	pthread_mutex_unlock(&list_mutext);
}

void swap(struct List* first, struct List* second) {
//	pthread_mutex_lock(&list_mutext);
	char* tmp = first->str;
	first->str = second->str;
	second->str = tmp;
//	pthread_mutex_unlock(&list_mutext);
}

void* sort(void* args) {
	while(1) {
		pthread_mutex_lock(&list_mutext);
		struct List* list = ((struct List*)args)->next;
		struct List* cur = list;
		struct List* inner_cur = list;

		while(cur) {
			while(inner_cur) {
				if(strcmp(cur->str, inner_cur->str) > 0)
					swap(inner_cur, cur);
				inner_cur = inner_cur->next;
			}
			cur = cur->next;
		}
		pthread_mutex_unlock(&list_mutext);
		sleep(5);
	}
}

int main(int argc, char* argv[]){
	pthread_t thread;
    	int status;
    	int status_addr;
 	char* get = NULL;

	pthread_mutex_init(&list_mutext, NULL);
	struct List* list;
	list = create_head();

    	status = pthread_create(&thread, NULL, sort, (void*)list);

	while(1) {
		get = (char*)malloc(sizeof(char)*81);
		fgets(get, 80, stdin);
		printf("got : %s\n", get);

		if(strlen(get) == 1) {
			print_all(list);
			continue;
		}

		//if((get[0] == ".") && (strlen(get)==2))
		//		break;
		add(list, get);

	}

	pthread_cancel(thread);
	pthread_join(thread, NULL);
	pthread_mutex_destroy(&list_mutext);


	
	return 0;
}
