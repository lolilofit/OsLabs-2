#include <errno.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<netdb.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include<signal.h>
#include <fcntl.h>
#include <sys/file.h>
#include <pthread.h>

#define MAX_HEADER_SIZE 2048
#define MAX_BODY_SIZE 2048
int FDS_SIZE = 200;
extern int errno;


struct HttpParams {
	char path[MAX_HEADER_SIZE+1];
	char host[MAX_HEADER_SIZE+1];
	char method[30];
	char protocol[30];
};

struct HttpAnswer {
	char status[4];
};

struct List {
	char str[MAX_HEADER_SIZE+MAX_BODY_SIZE+1];
	int len;
	struct List* next;
};

struct ClientHostList {
	int client;
	int remote_host;
	int should_cache;
	char* url;
	struct CacheUnit* cache_unit;
};


struct CacheUnit {
	struct List* mes_head;
	struct List* last_mes;
	char* url;
	pthread_mutex_t  m;
	int* waiting_now;
	int waiting_num;
	int is_downloading;
	int id;
	struct CacheUnit* next;
};


struct Cache {
	struct CacheUnit* units_head;
	int max_id;
};


void add_mes(struct CacheUnit* unit, char* mes, int mes_len) {
	struct List* add_this;
	add_this = (struct List*)malloc(sizeof(struct List));
	(add_this->str)[0] = '\0';
	memcpy(add_this->str, mes, mes_len);
	(add_this->str)[mes_len] = '\0';
	add_this->len = mes_len;
	add_this->next = NULL;

	pthread_mutex_lock(&(unit->m));
	unit->last_mes->next = add_this;
	unit->last_mes = unit->last_mes->next;
	pthread_mutex_unlock(&(unit->m));
}


struct CacheUnit* find_cache_by_url(struct Cache* cache, char* url) {
	printf("find in cache by url : %s, size %d\n", url, strlen(url));
	pthread_mutex_lock(&(cache->units_head->m));
	struct CacheUnit* cur = cache->units_head->next;
	pthread_mutex_unlock(&(cache->units_head->m));

	printf("head m lock\n");
	struct CacheUnit* prev;
	if(cur != NULL)
		pthread_mutex_lock(&(cur->m));
        while(cur != NULL) {
                if(strcmp(url, cur->url) == 0) {
			printf("FOUND in cache\n");
			pthread_mutex_unlock(&(cur->m));
            		return cur;
		}
		prev = cur;
                cur = cur->next;
		pthread_mutex_unlock(&(prev->m));
		if(cur != NULL)
			pthread_mutex_lock(&(cur->m));
        }
	printf("not found\n");
        return NULL;
}


struct CacheUnit* init_cache_unit(int id, char* url) {
 	struct CacheUnit* cache_unit;
	cache_unit = (struct CacheUnit*)malloc(sizeof(struct CacheUnit));
        cache_unit->id = id;
	cache_unit->next = NULL;
        cache_unit->waiting_num = 0;
	cache_unit->waiting_now = (int*)malloc(sizeof(int)*FDS_SIZE);
        cache_unit->is_downloading = 0;
	cache_unit->mes_head = (struct List*)malloc(sizeof(struct List));
	cache_unit->mes_head->next=NULL;
	cache_unit->last_mes = cache_unit->mes_head;
	(cache_unit->mes_head->str)[0] = '\0';
	cache_unit->mes_head->len = 0;

	if(pthread_mutex_init(&(cache_unit->m), NULL) != 0)
		printf("can't init mutex\n");

	cache_unit->url = url;
	return cache_unit;
}


struct Cache* init_cache(struct Cache* cache) {
	cache = (struct Cache*)malloc(sizeof(struct Cache));
	cache->max_id = 0;
	char* head_mes;
	head_mes = (char*)malloc(sizeof(char));
	head_mes[0] = '\0';
	cache->units_head = init_cache_unit(0, head_mes);
	return cache;
}


struct CacheUnit* add_cache_unit(struct Cache* cache, int id, char* url) {
        struct CacheUnit* cache_unit = NULL;
	pthread_mutex_lock(&(cache->units_head->m));
	struct CacheUnit* cur =  cache->units_head->next;

	if(cur == NULL) {
                        cache_unit = init_cache_unit(id, url);
                        cache->units_head->next = cache_unit;
			pthread_mutex_unlock(&(cache->units_head->m));
                        return cache_unit;
	}
	pthread_mutex_unlock(&(cache->units_head->m));

	struct CacheUnit* prev;
	if(cur != NULL)
		pthread_mutex_lock(&(cur->m));
	while(cur != NULL) {
		if(strcmp(cur->url, url) == 0) {
			 pthread_mutex_unlock(&(cur->m));
			return NULL;
		}
		if(cur->next == NULL) {
			cache_unit = init_cache_unit(id, url);
			cur->next = cache_unit;
			pthread_mutex_unlock(&(cur->m));
			return cache_unit;
		}
		prev = cur;
		cur = cur->next;
		pthread_mutex_unlock(&(prev->m));
		if(cur != NULL)
			pthread_mutex_lock(&(cur->m));
	}
	
	return cache_unit;
}


void dealloc_cache(struct Cache* cache) {
	struct CacheUnit* unit = cache->units_head;
	struct CacheUnit* prev;
	while(unit) {
		struct List* cur_list = unit->mes_head;
		while(cur_list != NULL) {
			free(cur_list);
			cur_list=cur_list->next;
		}
		free(unit->url);
		free(unit->waiting_now);
		pthread_mutex_destroy(&(unit->m));
		prev = unit;
		unit = unit->next;
		free(prev);
	}
}

int add_waiting(struct CacheUnit* cache_unit, int client) {
	printf("add waiting\n");
	if(cache_unit == NULL)
		return -1;
	cache_unit->waiting_now[cache_unit->waiting_num] = client;
	(cache_unit->waiting_num)++;
	return 0;
}

struct Cache* cache;

void pars_path(struct HttpParams* response) {
	char* tmp;
	tmp  = strstr(response->path, response->host);
	if(tmp != NULL) {
		char *end = tmp  + strlen(response->host);
		strcpy(response->path, end);
	}
}

void pars_line(char* line, struct HttpParams* response, int is_first) {
	char* ptr;
	int flag = 0, i = 0;

        ptr = strtok(line, " ");

	if(is_first == 1)
		strcpy(response->method, ptr);

	while(ptr != NULL) {
		if(flag == 1) 
			strcpy(response->host, ptr);
                if(strcmp(ptr, "Host:") == 0)
			flag = 1;

		if(i == 1 && is_first == 1)
			 strcpy(response->path, ptr);
		if(i == 2 && is_first == 1)
			strcpy(response->protocol, ptr);
		i++;
                ptr = strtok(NULL, " ");
        }
}

void pars_answer_line(char* line,  struct HttpAnswer* remote_answer, int is_first) {
	char* ptr;
        int flag = 0, i = 0;

        ptr = strtok(line, " ");
        while(ptr != NULL) {
                if(i == 1 && is_first == 1)
                         strcpy(remote_answer->status, ptr);
		i++;
                ptr = strtok(NULL, " ");
        }
}


void parse_request(char* request, struct HttpParams* response, struct HttpAnswer* remote_answer) {
	int i = 0, flag = 0;
	char* ptr;
	char tmp[MAX_HEADER_SIZE+1];

	struct List* head;
	head = (struct List*)malloc(sizeof(struct List));
	head->next = NULL;

	struct List* cur = head;

	ptr = strtok(request, "\r\n");

	while(ptr != NULL) {
		struct List* el;
		el = (struct List*)malloc(sizeof(struct List));
		strcpy(el->str, ptr);
		el->next = NULL;
		cur->next = el;
		cur = el;
		if(remote_answer != NULL)
			ptr = NULL;
		else
			ptr = strtok(NULL, "\r\n");
	}

	head = head->next;
	while(head != NULL) {
		if(flag == 0) {
			if(response != NULL)
				pars_line(head->str, response, 1);
			if(remote_answer != NULL)
				pars_answer_line(head->str, remote_answer, 1);
			flag = 1;
		}
		else {
			if(response != NULL)
				pars_line(head->str, response, 0);
		}
		cur = head;
		head = head->next;
		free(cur);
	}

	if(response != NULL) {
		pars_path(response);
		printf("path is %s\n", response->path);
	}
}



int get_remote_socket(char* host, char* port) {
	int remote_port = 80, sc, res;
	struct sockaddr_in sc_addr;

	if(strlen(port) > 0)
		remote_port = atoi(port);
	struct hostent *hp;
	hp = gethostbyname(host);
	if(hp == NULL) {
		printf("wrong url\n");
		return -1;
	}

	sc = socket(AF_INET, SOCK_STREAM, 0);
	if(sc < 0) {
		printf("can't create remote socket\n");
		return -1;
	}

	sc_addr.sin_family = AF_INET;
        sc_addr.sin_addr.s_addr = ((struct in_addr*)((hp->h_addr_list)[0]))->s_addr;
	printf("remote_adr is: %s\n", inet_ntoa(sc_addr.sin_addr));
	sc_addr.sin_port = htons(remote_port);

	bzero(&(sc_addr.sin_zero), 8);
        if((res = connect(sc, (struct sockaddr*)&sc_addr, sizeof(sc_addr))) < 0) {
		printf("can't connect to the remote socket\n");
		return -1;
	}
        struct timeval tv;
        tv.tv_sec = 5;
	tv.tv_usec = 0;
        setsockopt(sc, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
	return sc;
}



int transfer_to_waiters(struct CacheUnit* cache_unit) {
	printf("transfer to waiters\n");
	if(cache_unit == NULL)
		return -1;
	if(cache_unit->waiting_now == NULL)
		return -1;

	int i, client;
	for(i = 0; i < cache_unit->waiting_num; i++) {
		client = (cache_unit->waiting_now)[i];
		struct List* cur = cache_unit -> mes_head;
		while(cur != NULL) {
			if(write(client, cur->str, cur->len) < 0) {
                       		printf("can't write to waiting client\n");
                        	return -1;
                	}
		}
		close(client);
	}
	(cache_unit->waiting_num) = 0;
	return 0;
}

int transfer_cached(struct CacheUnit* cache_unit, int client) {
	printf("transfer to particular waiter\n");
	 struct List* cur = cache_unit -> mes_head->next;
                while(cur != NULL) {
			printf("%s", cur->str);
                        if(write(client, cur->str, cur->len) < 0) {
                                printf("can't write to remote host\n");
                                return -1;
                        }
                	cur = cur->next;
		}
	return 0;
}


int transfer_to_remote(struct ClientHostList* related) {

	char buf[MAX_HEADER_SIZE + MAX_BODY_SIZE+1];
	buf[0] = '\0';
	int remote_host, client;
	client = related->client;
	int readen = recv(client, buf, 2048, 0);
        if(readen < 0) {
                printf("error reading\n");
                return -1;
        }
	if(readen == 0) {
		printf("can't read anything (readed 0 bytes)\n");
		return -1;
	}
	buf[readen] = '\0';
        struct HttpParams* param;
        param = (struct HttpParams*)malloc(sizeof(struct HttpParams));
	char copy[readen];
        memcpy(copy, buf, readen);
	copy[readen] = '\0';
	parse_request(copy, param, NULL);

	char* url;
        url = (char*)malloc(sizeof(char)*readen);
        url[0] = '\0';

	if(param->path != NULL && param->host != NULL) {
		strncat(url, param->host, readen);
		strncat(url, param->path, readen);
	}
	struct CacheUnit* found = find_cache_by_url(cache, url);
	printf("found end\n");

	if((strcmp(param->method, "GET") == 0) || (strcmp(param->method, "HEAD") == 0)) {
	if(found != NULL) {
		pthread_mutex_lock(&(found->m));
		if(found->is_downloading == 0) {
			printf("it's in cache and downloaded, transfer to %d\n", related->client);
			if(transfer_cached(found, related->client) < 0) {
				pthread_mutex_unlock(&(found->m));
				return -1;
			}
			pthread_mutex_unlock(&(found->m));
			return 1;
		}
		else {
			printf("in cache and not downloaded\n");
			if(add_waiting(found, related->client) < 0) {
				pthread_mutex_unlock(&(found->m));
				return -1;
			}
			pthread_mutex_unlock(&(found->m));
			return 2;
		}
	}
	else {
		printf("created unit will be added to cache (probably)\n");
		related->should_cache = 1;
		related->url = (char*)malloc(sizeof(char)*(strlen(url)+1));
		(related->url)[0] = '\0';
		strcpy(related->url, url);
	}
	}
	else {
		related->should_cache = 0;
	}
	free(url);

	remote_host = get_remote_socket(param->host, "");
	if(remote_host < 0) {
             	return -1;
        }

	printf("Send this: %s\n", buf);
	if(write(remote_host, buf, readen) < 0) {
	       printf("can't write to remote host\n");
                return -1;
        }
	related->remote_host = remote_host;
	related->cache_unit = NULL;
	free(param);
	return 0;
}


int transfer_back(struct ClientHostList* related) {

	char buf[MAX_HEADER_SIZE + MAX_BODY_SIZE+1] = "";
	int remote_host = related->remote_host;
	int client = related->client;
	int readen = 0;
	int  status, client_alive = 1;
	buf[0] = '\0';

        readen = read(remote_host, buf, MAX_HEADER_SIZE+MAX_BODY_SIZE);
	buf[readen] = '\0';

	if(readen < 0) {
                printf("error reading from remote host\n");
                return -1;
        }

	if(readen == 0) { 
		printf("nothing to read anymore\n");
		return 1;
	}

        if(write(client, buf, readen) < 0) {
                printf("can't write to remote host\n");
               	client_alive = 0;
        }
        struct HttpAnswer* ans;
        ans = (struct HttpAnswer*)malloc(sizeof(struct HttpAnswer));
	char* pos = strchr(buf, '\r');
	if(pos != NULL) {
		int line_end = pos - buf;
		char copy[line_end+1];
		strncpy(copy, buf, line_end);
		copy[line_end] = '\0';
		parse_request(copy, NULL, ans);
	}
	printf("answer is: %d\n", atoi(ans->status));
	if(related->should_cache == 1) {
                struct CacheUnit* found = find_cache_by_url(cache, related->url);

	        if(found == NULL) {
		if(atoi(ans->status) == 200) {
			printf("let's add to cache\n");
			(cache->max_id)++;
			related->cache_unit = add_cache_unit(cache, cache->max_id, related->url);
			add_mes(related->cache_unit, buf, readen);
		}
		}
	}
	free(ans);

	while(1) {
		buf[0] = '\0';
		readen = read(remote_host, buf, MAX_HEADER_SIZE+MAX_BODY_SIZE);
		if(readen < 0) {
			if(errno == EWOULDBLOCK)
				break;
                        printf("error reading from remote host-additional read, remote_host %d\n", remote_host);
			return 1;
                }
		if(readen == 0) {
			break;
		}
                buf[readen]='\0';
		if(related->cache_unit != NULL)
			add_mes(related->cache_unit, buf, readen);

		if(client_alive == 1) {
			if(write(client, buf, readen) < 0) {
                		printf("can't write to client\n");
        	        	client_alive = 0;
	        	}
		}

        }

	if(related->cache_unit != NULL) {
		pthread_mutex_lock(&(related->cache_unit->m));
		related->cache_unit->is_downloading = 0;
		transfer_to_waiters(related->cache_unit);
		pthread_mutex_unlock(&(related->cache_unit->m));
	}
	return 1;
}

void* transfer(void *args) {
	int client = (int)(args);
	struct ClientHostList related;
	related.client = client;
	related.remote_host = -1;
	related.cache_unit = NULL;
	
	while(related.client == client) {
		if(transfer_to_remote(&related) == -1) {
		 	close(related.client);
			if(related.remote_host > 0)
				close(related.remote_host);
			related.client = -1;
			related.remote_host = -1;
			break;
		}
		transfer_back(&related);
		if(related.remote_host > 0)
			close(related.remote_host);
	}
	return;
}


int main(int argc, char* argv[]) {
	cache = init_cache(cache);
	if(cache == NULL) {
		printf("null cache\n");
	}

	int i, sc, client, remote_host, ret, res;
	struct sockaddr_in sc_addr;
	int addrlen = sizeof(sc_addr);
	sc = socket(AF_INET, SOCK_STREAM, 0);
	if(sc == -1) {
		close(sc);
		printf("can't create socket\n");
		return 1;
	}
	int opt = 1;
	if (setsockopt(sc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) { 
        	printf("setsockopt error");
        	exit(-1);
    	}

	sc_addr.sin_family = AF_INET;
        sc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sc_addr.sin_port = htons(5000);
	if(bind(sc, (struct sockaddr*)&sc_addr, sizeof(sc_addr)) < 0) {
		printf("can't bind");
		return -1;
	}

	if(listen(sc, SOMAXCONN) < 0) {
		printf("errir listen\n");
		return -1;
	}

	while(1) {

		if((client = accept(sc, (struct sockaddr*)&sc_addr, (socklen_t*)&addrlen)) < 0) {
                	printf("error accept\n");
                	continue;
		}
		pthread_t thread;
		int status;
		status = pthread_create(&thread, NULL, transfer, (void*)(client));
		if(status != 0) {
			fprintf(stderr, "Error creating thread\n");
		}
		pthread_detach(thread);
	}
	return 0;
}
