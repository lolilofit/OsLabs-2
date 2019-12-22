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
#include<unistd.h>

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
  pthread_mutex_t list_m;
};

struct ClientHostList {
  int client;
  int remote_host;
  int should_cache;
  char* url;
//  struct ClientHostList* next;
  struct CacheUnit* cache_unit;
  int is_cli_alive;
 // int is_host_done;
 // int is_first;
};

struct CacheUnit {
	struct List* mes_head;
	struct List* last_mes;
	char* url;
	pthread_mutex_t  m;
	struct CacheUnit* next;
};


struct Cache {
	struct CacheUnit* units_head;
	int max_id;
};


struct List* add_mes(struct CacheUnit* unit, char* mes, int mes_len) {
//	printf("add mes\n");
	struct List* add_this;
	add_this = (struct List*)malloc(sizeof(struct List));
  	pthread_mutex_init(&(add_this->list_m), NULL);

	(add_this->str)[0] = '\0';
	memcpy(add_this->str, mes, mes_len);
	(add_this->str)[mes_len] = '\0';
	add_this->len = mes_len;
	add_this->next = NULL;

/*
  if(unit->last_mes != unit->mes_head) {
	pthread_mutex_lock(&(unit->last_mes->list_m));
	//pthread_mutex_t* mut = &(unit->last_mes->list_m);
  struct List* saved = unit->last_mes;

	unit->last_mes->next = add_this;
	unit->last_mes = unit->last_mes->next;
	pthread_mutex_unlock(&(saved->list_m));
  //pthread_mutex_unlock(mut);
  }
  else {
*/  
  	unit->last_mes->next = add_this;
	unit->last_mes = unit->last_mes->next;  
  //}
//	printf("add mes end\n");
  return add_this;
}


struct CacheUnit* find_cache_by_url(struct Cache* cache, char* url) {
	printf("find in cache by url : %s, size %d\n", url, strlen(url));
	if(pthread_mutex_lock(&(cache->units_head->m)) != 0) {
    	printf("SOMETING WRONG wit mutex\n");
  	}
	//printf("find in cache head locked\n");
  	struct CacheUnit* cur = cache->units_head->next;
  	//printf("find in cache head unlocked\n");

	//printf("head m lock\n");
	struct CacheUnit* prev;
	if(cur != NULL)
		pthread_mutex_lock(&(cur->m));
	pthread_mutex_unlock(&(cache->units_head->m));

    //printf("find mut lock\n");
        while(cur != NULL) {
  //        printf("compare\n");
          if(strcmp(url, cur->url) == 0) {
			      printf("FOUND in cache\n");
			      pthread_mutex_unlock(&(cur->m));
      //        printf("find mut Unlock\n");
            return cur;
		      }
		      prev = cur;
          cur = cur->next;
        //  printf("find mut Unlock\n");
		      if(cur != NULL) {
			      pthread_mutex_lock(&(cur->m));
          //  printf("find mut lock\n");
          }
          pthread_mutex_unlock(&(prev->m));
        }
      printf("not found\n");
      return NULL;
}


struct CacheUnit* init_cache_unit(char* url) {
  struct CacheUnit* cache_unit;
  cache_unit = (struct CacheUnit*)malloc(sizeof(struct CacheUnit));
  cache_unit->next = NULL;
  cache_unit->mes_head = (struct List*)malloc(sizeof(struct List));
  pthread_mutex_init(&(cache_unit->mes_head->list_m), NULL);
  cache_unit->mes_head->next=NULL;

  pthread_mutex_init(&(cache_unit->m), NULL);
  cache_unit->last_mes = cache_unit->mes_head;
  (cache_unit->mes_head->str)[0] = '\0';
  cache_unit->mes_head->len = 0;
  cache_unit->url = url;
  return cache_unit;
}


struct Cache* init_cache(struct Cache* cache) {
  cache = (struct Cache*)malloc(sizeof(struct Cache));
  cache->max_id = 0;
  char* head_mes = (char*)malloc(sizeof(char));
  head_mes[0] = '\0';
  cache->units_head = init_cache_unit(head_mes);
  return cache;
}

struct CacheUnit* add_cache_unit(struct Cache* cache, char* url, struct CacheUnit* cache_unit) {
  
	pthread_mutex_lock(&(cache->units_head->m));
	//printf("add_cache_unit head lock\n");
 	 struct CacheUnit* cur =  cache->units_head->next;

	if(cur == NULL) {
    	 	cache->units_head->next = cache_unit;
		pthread_mutex_unlock(&(cache->units_head->m));
    //  printf("add_cache_unit Unlock\n");
      		return cache_unit;
	}

  //printf("add_cache_unit Unlock\n");
   struct CacheUnit* prev;

  if(cur != NULL) {
  	pthread_mutex_lock(&(cur->m));
  // 	printf("add_cache_unit lock\n");
  }

    pthread_mutex_unlock(&(cache->units_head->m));


  while(cur != NULL) {
		if(strcmp(cur->url, url) == 0) {
			 pthread_mutex_unlock(&(cur->m));
    //   printf("add_cache_unit Unlock\n");
			return NULL;
		}
		if(cur->next == NULL) {
			cur->next = cache_unit;
			pthread_mutex_unlock(&(cur->m));
      //printf("add_cache_unit Unlock\n");
			return cache_unit;
		}
		prev = cur;
		cur = cur->next;
    if(cur != NULL) {
			pthread_mutex_lock(&(cur->m));
    //  printf("add_cache_unit lock\n");
    }
		pthread_mutex_unlock(&(prev->m));
    //printf("add_cache_unit Unlock\n");
	}

	return cache_unit;
//	return NULL;
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

struct Host{
        char host[MAX_HEADER_SIZE+1];
        int port;
};

int divide_host(char* response, struct Host* host) {
        char *tmp;
        char port[MAX_HEADER_SIZE+1];

        tmp = strstr(response, ":");
        if(tmp != NULL){
                strcpy(port, tmp + strlen(":"));
                strncpy(host->host, response, (tmp-response));
                (host->host)[tmp-response] = '\0';
                host->port = atoi(port);
        }
        else {
                strcpy(host->host, response);
                host->port = 80;
        }
        return 0;
}

int get_remote_socket(char* host, char* port, struct Host* clear_host) {
        int remote_port = 80, sc, res;
        struct sockaddr_in sc_addr;

        struct hostent *hp;
        hp = gethostbyname(clear_host->host);
        if(hp == NULL) {
                printf("wrong url\n");
                return -1;
        }

        sc = socket(PF_INET, SOCK_STREAM, 0);
        if(sc < 0) {
                printf("can't create remote socket\n");
                return -1;
        }

        sc_addr.sin_family = AF_INET;
        sc_addr.sin_addr.s_addr = ((struct in_addr*)((hp->h_addr_list)[0]))->s_addr;
        printf("remote_adr is: %s, CONNECT TO %s, %d\n", inet_ntoa(sc_addr.sin_addr), clear_host->host, clear_host->port);

        sc_addr.sin_port = htons(clear_host->port);
        if((res = connect(sc, (struct sockaddr*)&sc_addr, sizeof(sc_addr))) < 0) {
                printf("can't connect to the remote socket\n");
                return -1;
        }
        struct timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        setsockopt(sc, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
        return sc;
}

int transfer_cached(struct CacheUnit* cache_unit, int client) {
	printf("transfer to particular waiter\n");
  if(cache_unit == NULL) {
    printf("NULL cache unit in transfer\n");
    return 0;
  }
  if(cache_unit->mes_head == NULL) {
    printf("NULL mes head in cache unit in transfer\n");
    return 0;
  }

  pthread_mutex_lock(&(cache_unit->mes_head->list_m));

  //printf("trans_cached lock");
  struct List* cur = cache_unit -> mes_head->next;
  if(cur != NULL) {
    pthread_mutex_lock(&(cur->list_m)); 
    printf("trans_cached unlock");
  }
  pthread_mutex_unlock(&(cache_unit->mes_head->list_m));

  while(cur != NULL) {
		  if(write(client, cur->str, cur->len) < 0) {
    //      printf("can't write to remote host\n");
          pthread_mutex_unlock(&(cur->list_m));
      //    printf("trans_cached unlock");
          return -1;
      }
      struct List* prev = cur;
      cur = cur->next;
      if(cur != NULL) {
        pthread_mutex_lock(&(cur->list_m));
       // printf("trans_cached lock");
      }
      pthread_mutex_unlock(&(prev->list_m));
  }

  //pthread_mutex_unlock(&(cache_unit->m));
  //printf("trans_cached final unlock");
	return 0;
}

void resolveEmtyHost(struct HttpParams* param) {
  if(param->path != NULL) {
    char *http;
    http = strstr(param->path, "//");
    if(http != NULL) {
      char* slash;
      slash = strstr(http + 2, "/");
      if(slash != NULL) {
      strncpy(param->host, http+2, (slash - http - 2));
      (param->host)[slash-http-2] = '\0';
      strcpy(param->path, slash + 1);
    }
    else {
       strcpy(param->host, http+2);
       strcpy(param->path, "/");
      }
    }
  }
  else {
      char* slash;
      slash = strstr(param->path, "/");
      if(slash != NULL) {
        strncpy(param->host, param->path, (slash - param->path));
        strcpy(param->path, slash + 1);
      }
      else {
        strcpy(param->host, param->path);
        strcpy(param->path, "/");
      } 
    }                 
}


int transfer_to_remote(struct ClientHostList* related) {
  char buf[MAX_HEADER_SIZE + MAX_BODY_SIZE+1];
  buf[0] = '\0';
  int remote_host, client;
  client = related->client;
  int readen = 0;
  readen = recv(client, buf, 2048, 0);
  if(readen < 0) {
    printf("error reading\n");
    return -1;
  }
  if(readen == 0) {
    return -1;
  }
  buf[readen] = '\0';

  struct HttpParams* param;
  param = (struct HttpParams*)malloc(sizeof(struct HttpParams));
  char copy[readen];
  memcpy(copy, buf, readen);
  copy[readen] = '\0';
  parse_request(copy, param, NULL);

  struct Host h;
  h.host[0] = '\0';
  h.port = -1;

  if(param->host != NULL) {
    if((param->host)[0] != '\0')
        divide_host(param->host, &h);
    else {
        resolveEmtyHost(param);
        divide_host(param->host, &h);
        printf("\n\nNO HOST IN HEADER end with 0 \n\n");
    }
  }
  else {
      resolveEmtyHost(param);
      divide_host(param->host, &h);
      printf("\n\n NO HOST IN HEADER end null\n\n");
      printf("Host is: %s, path %s\n", param->host, param->path);
  }

  char* url;
  url = (char*)malloc(sizeof(char)*readen);
  url[0] = '\0';

  if(h.host != NULL)
    strncat(url, h.host, readen);
  if(param->path != NULL)
    strncat(url, param->path, readen);
        //printf("BEFORe find in cache by url %s, client %d, host %d \n", url, related->client, related->remote_host);
  struct CacheUnit* found = find_cache_by_url(cache, url);
  printf("found end\n");

  if((strcmp(param->method, "GET") == 0) || (strcmp(param->method, "HEAD") == 0)) {
      if(found != NULL) {
        printf("it's in cache and downloaded, transfer to %d\n", related->client);
        if(transfer_cached(found, related->client) < 0)
          return -1;
        return 1;
      }
      else {
        printf("created unit will be added to cache (probably)\n");
        related->should_cache = 1;
        related->url = (char*)malloc(sizeof(char)*readen);
        (related->url)[0] = '\0';
        strncpy(related->url, url, readen);
      }
  }
  else {
        related->should_cache = 0;
  }
  free(url);

  remote_host = get_remote_socket(param->host, "", &h);
  if(remote_host < 0) {
    return -1;
  }

  if(write(remote_host, buf, readen) < 0) {
    printf("can't write to remote host\n");
    return -1;
  }
    related->remote_host = remote_host;
    related->cache_unit = NULL;
//    related->is_first = 1;   
    free(param);
    return 0;
}


int transfer_back(struct ClientHostList* related)  {

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
//    related->is_host_done = 1;
	printf("host is done\n");
    return 1;
  }
  if(write(client, buf, readen) < 0) {
    printf("can't write to remote host\n");
    related->is_cli_alive = 0;
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
  if(related->url != NULL)
    printf("answer is: %d, url: %s, client %d, host %d\n", atoi(ans->status), related->url, related->client, related->remote_host);
  
  struct List* prev_block = NULL;
  struct List* current_block = NULL;

    if(related->should_cache == 1) {
    //struct CacheUnit* found = find_cache_by_url(cache, related->url);
    //if(found == NULL) {
        if(atoi(ans->status) == 200) {
          printf("let's add to cache\n");
            (cache->max_id)++;
            struct CacheUnit* cache_unit = init_cache_unit(related->url);
            if(cache_unit != NULL) {
              pthread_mutex_lock(&(cache_unit->mes_head->list_m));
              related->cache_unit = add_cache_unit(cache, related->url, cache_unit);
            } 
	    if(related->cache_unit == NULL) {
               pthread_mutex_unlock(&(cache_unit->mes_head->list_m));
            }
	    else {
	       prev_block = add_mes(related->cache_unit, buf, readen);
	       pthread_mutex_lock(&(prev_block->list_m));
	       pthread_mutex_unlock(&(cache_unit->mes_head->list_m));
	   }
	   }
      //}
    }
  free(ans);
  
  while(1) {
      buf[0] = '\0';
//      printf("Let's read\n");
      readen = read(remote_host, buf, MAX_HEADER_SIZE+MAX_BODY_SIZE);
  //    printf("Read finished\n");
      if(readen < 0) {
          if(related->url != NULL)
                printf("error reading from remote host in while %s, total readed %d\n", related->url);
          if(related->cache_unit != NULL) {
            pthread_mutex_unlock(&(prev_block->list_m));
  	  }
          return 1;
      }
      if(readen == 0) {
          if(related->cache_unit != NULL) {
            pthread_mutex_unlock(&(prev_block->list_m));
       //     printf("Head mut Unlock\n");
          }
          return  1;
    }
    buf[readen]='\0';
        if(related->cache_unit != NULL) {
		current_block = add_mes(related->cache_unit, buf, readen);
		pthread_mutex_lock(&(current_block->list_m));
		pthread_mutex_unlock(&(prev_block->list_m));
		prev_block = current_block;
	}

    if(related->is_cli_alive == 1) {
      if(write(client, buf, readen) < 0) {
              printf("can't write to client wrto %d, real %d\n", client, related->client);
             perror("cli fail");
             related->is_cli_alive = 0;
      }
    }
  }

  return  2;
}

void* transfer(void *args) {
	//struct sigaction sig;
	//sig.sa_handler = SIG_IGN;
	//sigaction(SIGPIPE, &sig, NULL);

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	int client = (int)(args);
	struct ClientHostList related;
	related.client = client;
	related.remote_host = -1;
	related.cache_unit = NULL;
	related.is_cli_alive = 1;
//	while(related.client == client) {
	while(1) {
		if(transfer_to_remote(&related) == -1) {
		 	close(related.client);
			if(related.remote_host > 0)
				close(related.remote_host);
			related.client = -1;
			related.remote_host = -1;
			break;
		}
		transfer_back(&related);
		if(related.remote_host > 0) {
			close(related.remote_host);
			related.remote_host = -1;
		}
		
		printf("CYCLE!!!!!\n");
	}
	return 0;
}


int main(int argc, char* argv[]) {
       //struct sigaction sig;
       //sig.sa_handler = SIG_IGN;
       //sigaction(SIGPIPE, &sig, NULL);
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

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
