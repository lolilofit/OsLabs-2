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

#define MAX_HEADER_SIZE 2048
#define MAX_BODY_SIZE 2048
int FDS_SIZE = 200;



struct HttpParams {
	char path[MAX_HEADER_SIZE+1];
	char host[MAX_HEADER_SIZE+1];
	char method[30];
	char protocol[30];
};

struct HttpAnswer {
	int content_lengh;
	char status[4];
	char body[MAX_BODY_SIZE+1];
	char header[MAX_HEADER_SIZE+1];
	int chunked;
};

struct List {
	char str[MAX_HEADER_SIZE+MAX_BODY_SIZE+1];
	struct List* next;
};

struct ClientHostList {
	int client;
	int remote_host;
	int should_cache;
	char* url;
	struct ClientHostList* next;
	int answer_parsed;
	struct CacheUnit* cache_unit;
};


struct CacheUnit {
	struct List* mes_head;
	struct List* last_mes;
	char* url;
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
	printf("add new message to list\n");
	struct List* add_this;
	add_this = (struct List*)malloc(sizeof(struct List));
	(add_this->str)[0] = '\0';
	strncpy(add_this->str, mes, mes_len);
	(add_this->str)[mes_len] = '\0';
	add_this->next = NULL;

	unit->last_mes->next = add_this;
	unit->last_mes = unit->last_mes->next;
	printf("add new message to list end\n");
}


struct CacheUnit* find_cache_by_id(struct Cache* cache, int id) {
	struct CacheUnit* cur = cache->units_head->next;
	while(cur != NULL) {
		if(cur->id == id)
			return cur;
		cur = cur->next;
	}
	return NULL;
}

struct CacheUnit* find_cache_by_url(struct Cache* cache, char* url) {
	printf("find in cache by url : %s, size %d\n", url, strlen(url));
	struct CacheUnit* cur = cache->units_head->next;
        while(cur != NULL) {
       		printf("compare|%s|and|%s|\n", url , cur->url);
                if(strcmp(url, cur->url) == 0) {
			printf("FOUND in cache\n");
            		return cur;
		}
                cur = cur->next;
        }
	printf("not found\n");
        return NULL;
}


struct CacheUnit* init_cache_unit(int id, char* url, int url_size) {
	printf("init cache unit\n");
 	struct CacheUnit* cache_unit;
	cache_unit = (struct CacheUnit*)malloc(sizeof(struct CacheUnit));
        cache_unit->id = id;
	cache_unit->next = NULL;
        cache_unit->waiting_num = 0;
	cache_unit->waiting_now = (int*)malloc(sizeof(int)*FDS_SIZE);
        cache_unit->is_downloading = 1;
	cache_unit->mes_head = (struct List*)malloc(sizeof(struct List));
	cache_unit->mes_head->next=NULL;
	cache_unit->last_mes = cache_unit->mes_head;
	(cache_unit->mes_head->str)[0] = '\0';

	if(url_size != 0) {
		 cache_unit->url = (char*)malloc(sizeof(char)*url_size);
		 strncpy(cache_unit->url, url, url_size);
	}
	else {
		 cache_unit->url=NULL;
	}
	printf("init cache unit end\n");
	return cache_unit;
}




struct Cache* init_cache(struct Cache* cache) {
	printf("init cache\n");
	cache = (struct Cache*)malloc(sizeof(struct Cache));
	cache->max_id = 0;
	cache->units_head = init_cache_unit(0, "", 0);
	printf("init cache end\n");
	return cache;
}


struct CacheUnit* add_cache_unit(struct Cache* cache, int id, char* url, int url_size) {
	printf("add unit to cache\n");
        struct CacheUnit* cache_unit = NULL;
	struct CacheUnit* cur =  cache->units_head->next;
	if(cur == NULL) {
                        cache_unit = init_cache_unit(id, url, url_size);
                        cache->units_head->next = cache_unit;
                        return cache_unit;
	}

	while(cur != NULL) {
		if(strcmp(cur->url, url) == 0)
			return NULL;
		if(cur->next == NULL) {
			cache_unit = init_cache_unit(id, url, url_size);
			cur->next = cache_unit;
			return cache_unit;
		}
		cur = cur->next;
	}

	printf("add unit to cache end\n");
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
	printf("add waiting end\n");
	return 0;
}

int divide_body(char* response, struct HttpAnswer* answer) {
	char *tmp;
	char part[MAX_HEADER_SIZE+1];

	tmp = strstr(response, "\r\n\r\n");
	if(tmp != NULL){
		strcpy(answer->body, tmp + strlen("\r\n\r\n"));
		strncpy(answer->header, response, (tmp-response));
	}
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
		if(flag == 1)
			remote_answer->content_lengh = atoi(ptr);
                if(strcmp(ptr, "Content-Length:") == 0)
                        flag = 1;
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

	  	if(remote_answer != NULL) {
			if(strcmp("Transfer-Encoding: chunked", ptr) == 0)
				remote_answer->chunked=1;
		}
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
		//	 if(remote_answer != NULL)
                //                pars_answer_line(head->str, remote_answer, 0);
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
	int fileflags;
        if (fileflags = fcntl(sc, F_GETFL, 0) == -1) {
                perror("fcntl F_GETFL");
                exit(1);
        }
        if (fcntl(sc, F_SETFL, fileflags | FNDELAY) == -1) {
                perror("fcntl F_SETFL, FNDELAY");
                exit(1);
        }

	return sc;
}


void form_http_request(struct HttpParams* response, char* header) {
	sprintf(header, "%s %s %s\r\nHost: %s\r\n\r\n", response->method, response->path, "HTTP/1.1", response->host);
}

struct ClientHostList* find_related(struct ClientHostList* head, int find_him) {

	struct ClientHostList* cur = head->next;
	struct ClientHostList* prev = head;
	while(cur != NULL) {
		if((cur->client == find_him) || (cur->remote_host == find_him))
			return prev;
		cur = cur->next;
		prev = prev->next;
	}
	return NULL;
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
			//transfer to the next
			if(write(client, cur->str, strlen(cur->str)) < 0) {
                       		printf("can't write to waiting client\n");
                        	return -1;
                	}
		}
		close(client);
	}
	(cache_unit->waiting_num) = 0;
	printf("transfer to waiters end\n");
	return 0;
}

int transfer_cached(struct CacheUnit* cache_unit, int client) {
	printf("transfer to particular waiter\n");
	 struct List* cur = cache_unit -> mes_head->next;
                while(cur != NULL) {
			printf("%s\n", cur->str); 
                       if(write(client, cur->str, strlen(cur->str)) < 0) {
                                printf("can't write to remote host\n");
                                return -1;
                        }
                	cur = cur->next;
		}
	 printf("transfer to particular waiter end\n");
	return 0;
}


int transfer_to_remote(struct ClientHostList* related, struct pollfd* fds, int nfd, int should_open) {

	printf("transfer to remote\n");
	char buf[MAX_HEADER_SIZE + MAX_BODY_SIZE+1];
	int remote_host, client;
	client = related->client;
	int readen = recv(client, buf, 2048, 0);
        if(readen < 0) {
                printf("error reading\n");
                return -1;
        }
	//??
	if(readen == 0) {
		printf("can't read anything (readed 0 bytes)\n");
		return -1;
	}

        printf("%s\n\n", buf);

        struct HttpParams* param;
        param = (struct HttpParams*)malloc(sizeof(struct HttpParams));
	char copy[readen];
        strcpy(copy, buf);
        parse_request(copy, param, NULL);


	//??sizes
	char url[MAX_HEADER_SIZE+ 1] = "";
	if(param->path != NULL && param->host != NULL) {
		strncat(url, param->host, MAX_HEADER_SIZE/2);
		strncat(url, param->path, MAX_HEADER_SIZE/2);
	}

	struct CacheUnit* found = find_cache_by_url(cache, url);
	printf("found end\n");
	if((strcmp(param->method, "GET") == 0) || (strcmp(param->method, "HEAD") == 0)) {
	if(found != NULL) {
		//impl
		if(found->is_downloading == 0) {
			printf("it's in cache and downloaded, transfer to %d\n", related->client);
			if(transfer_cached(found, related->client) < 0)
				return -1;
			return 1;
		}
		else {
			printf("in cache and not downloaded\n");
			if(add_waiting(found, related->client) < 0)
				return -1;
			return 2;
		}
	}
	else {
		printf("created unit will be added to cache\n");
		//(cache->max_id)++;
        	//add_cache_unit(cache, cache->max_id, url, strlen(url));
		related->should_cache = 1;
		related->url = (char*)malloc(sizeof(char)*(strlen(url)+1));
		(related->url)[0] = '\0';
		strcpy(related->url, url);
	}
	}
	else {
		related->should_cache = 0;
	}

//	if(should_open == 1) {
		remote_host = get_remote_socket(param->host, "");
	       	if(remote_host < 0) {
             		return -1;
        	}
//	}
//	else {remote_host = related->remote_host;}

        char* send_this;
        send_this = (char*)malloc(sizeof(char)*3*MAX_HEADER_SIZE);
        printf("form hhtp request\n\n");
        form_http_request(param, send_this);

        printf("send this: %s\n", buf);
        //if(write(remote_host, send_this, strlen(send_this)) < 0) {
        if(write(remote_host, buf, readen) < 0) { 
	       printf("can't write to remote host\n");
                return -1;
        }
	related->remote_host = remote_host;
	related->answer_parsed = 0;
	related->cache_unit = NULL;
	fds[nfd].fd = remote_host;
	fds[nfd].events = POLLIN;
	free(param);
	//free(send_this);
	return 0;
}


int transfer_back(struct ClientHostList* related) {
//	struct CacheUnit* cache_unit = NULL;

	char buf[MAX_HEADER_SIZE + MAX_BODY_SIZE+1] = "";
	int remote_host = related->remote_host;
	int client = related->client;
	int try_read = 0, readen, status, client_alive = 1;

	if(related->answer_parsed == 0) {
        readen = read(remote_host, buf, MAX_HEADER_SIZE+MAX_BODY_SIZE);
	buf[readen+1] = '\0';

	if(readen < 0) {
                printf("error reading from remote host");
                return -1;
        }

	if(readen == 0) { 
		printf("\n nothing to read anymore\n");
		return 1;
	//	return 2;
	}

        if(write(client, buf, readen) < 0) {
                printf("can't write to remote host\n");
               	client_alive = 0;
		 //return -1;
        }
        struct HttpAnswer* ans;
        ans = (struct HttpAnswer*)malloc(sizeof(struct HttpAnswer));
        (ans->body)[0] = '\0';
	ans->chunked = 0;
	ans->content_lengh = -1;
	//divide_body(buf, ans);
        //try_read+=strlen(ans->body);
        //parse_request(ans->header, NULL, ans);


	char* pos = strchr(buf, '\r');
	if(pos != NULL) {
		int line_end = pos - buf;
		printf("suze of line = %d\n", line_end);
		char copy[line_end+1];
		strncpy(copy, buf, line_end);
		copy[line_end] = '\0';
		printf("copy line = %s\n\n", copy);
		parse_request(copy, NULL, ans);
	}
	int flag = 0, dec_number = 0;

	//cache
	if(related->should_cache == 1) {
                struct CacheUnit* found = find_cache_by_url(cache, related->url);
                if(found == NULL) {
		if(atoi(ans->status) == 200) {
			printf("let's add to cache\n");
			(cache->max_id)++;
			related->cache_unit = add_cache_unit(cache, cache->max_id, related->url, strlen(related->url));
			add_mes(related->cache_unit, buf, readen);
		}
		}
	}
//	printf("%s\n\n", buf);
	related->answer_parsed = 1;
	free(ans);
	}

	printf("start reading\n");
	int close_flag = 0;
	while(1) {
  //      while(try_read < mes_len) {
		buf[0] = '\0';
		readen = read(remote_host, buf, MAX_HEADER_SIZE+MAX_BODY_SIZE);
		if(readen < 0) {
                        printf("error reading from remote host-additional read, remote_host %d\n", remote_host);
                        //related->answer_parsed = 0;
			//return -1;
			//close_flag = 1;
			break;
                }
		if(readen == 0) {
			//related->answer_parsed = 0;
			//close_flag = 1;
			printf("haven't read anything\n");
			break;
		}
                buf[readen+1]='\0';
		if(related->cache_unit != NULL)
			add_mes(related->cache_unit, buf, readen);

		if(client_alive == 1) {
			if(write(client, buf, readen) < 0) {
                		printf("can't write to client\n");
        	        	client_alive = 0;
				//what to return in the end
	        	}
		}

                //printf("again readen:\n\n%s", buf);
        }

	if(related->cache_unit != NULL) {
	 printf("show cached\n");
	struct List* t = related->cache_unit->mes_head;
	while(t != NULL) {
		printf("%s\n", t->str);
		printf("\n\nTRY to go to the next\n\n");
		t=t->next;
	}
	 printf("------------");
	}


	if(related->cache_unit != NULL) {
		related->cache_unit->is_downloading = 0;
		//related->cache_unit = NULL;
		//transfer_to_waiters(related->cache_unit);
	}

	if(close_flag == 1)
		return 1;
	return 2;
}


struct ClientHostList* remove_conn_info(struct pollfd* fds, int i, struct ClientHostList* prev, struct ClientHostList* related,  struct ClientHostList* last) {
	printf("remove_conn_info\n");
        if(related != NULL && prev != NULL) {
		prev->next = related->next;
        	free(related->url);
		free(related);
		if(prev->next == NULL)
			return prev;
	}
	return last;
}

int evacuate_fds(struct pollfd* fds) {
	int i, count = 0;
	for(i = 0; i < FDS_SIZE; i++) {
		if(fds[i].fd > 0) {
			if(count != i) {
				fds[count].fd = fds[i].fd;
				fds[count].events = POLLIN;
				fds[i].fd = -1;
			}
			count++;
		}
	}
	return count;
}

void cleanup(int sig) {
	dealloc_cache(cache);
}

struct pollfd* resize_fds( struct pollfd* fds) {
	struct pollfd* new_fds;
	FDS_SIZE = FDS_SIZE + 200;
	new_fds = realloc(fds, FDS_SIZE);
	if(new_fds == NULL) {
		printf("can't realloc clients fds\n");
		return fds;
	}
	return new_fds;
}

int main(int argc, char* argv[]) {
	cache = init_cache(cache);
//	signal(SIGINT, cleanup);

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
        	printf("setsockopt"); 
        	exit(-1); 
    	}
	int on = 1;
	if(ioctl(sc, FIONBIO, (char *)&on) < 0) {
		printf("can't make nonblocking");
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

	struct pollfd* fds;
	fds = (struct pollfd*)malloc(sizeof(struct pollfd)*FDS_SIZE);
	memset(fds, -1, sizeof(fds));
	fds[0].fd = sc;
  	fds[0].events = POLLIN;
	int timeout = 1*60*1000, nfd = 1, size_copy, j;

	struct ClientHostList* head;
	head = (struct ClientHostList*)malloc(sizeof(struct ClientHostList));
	head->client = -1;
	head->remote_host = -1;
	head->next = NULL;
	struct ClientHostList* last;
	last = head;

	while(1) {
		ret = poll(fds, nfd, timeout);
		if(ret < 0) {
			printf("poll failed\n");
			break;
		}
		if(ret == 0) {
			printf("exit after timeout\n");
			exit(1);
		}
		size_copy = nfd;
		for(i = 0; i < size_copy; i++) {
			if(fds[i].revents == 0) {
				continue;
			}
			//delete this fd
			if(fds[i].revents != POLLIN) {
				continue;
			}
			if(fds[i].fd == sc) {
				client = 0;
				while(client != -1) { 
					//printf("try accept self=%d, socket-to-acc=%d\n", sc, fds[i].fd);
					if((client = accept(fds[i].fd, (struct sockaddr*)&sc_addr, (socklen_t*)&addrlen)) < 0) {
                				printf("error accept\n");
                				continue;
			        	}
					printf("Client %d accepted\n\n", client);
					fds[nfd].fd = client;
					fds[nfd].events = POLLIN;
					//if > 200 => evacuate to copy alive desc
					nfd++;
					if(nfd >= FDS_SIZE)
						nfd = evacuate_fds(fds);
					if(nfd >= FDS_SIZE) {
						printf("clients overflow\n");
						fds = resize_fds(fds);
					}

					struct ClientHostList* new_client;
					new_client = (struct ClientHostList*)malloc(sizeof(struct ClientHostList));
					new_client->client = client;
					new_client->remote_host = -1;
					new_client->next=NULL;
					new_client->url = NULL;
					last->next = new_client;
					last = new_client;

					for(j = 0; j < nfd; j++) {
						printf("fd: %d\n", fds[j].fd);
					}
					printf("accepting sucess %d, nfd=%d\n", client, nfd);
				}
			}
			else {
					if(head == NULL)
						printf("head is null, should be allocated\n");
					struct ClientHostList* prev = find_related(head, fds[i].fd);
					if(prev == NULL) {
						printf("prev is null\n");
						fds[i].fd = -1;
						continue;
					}
					struct ClientHostList* related = prev->next;

					if(related->client == fds[i].fd) {
						printf("message came for %d (to host), and host is %d, client %d\n\n", fds[i].fd, related->remote_host, related->client);
						//if(related->remote_host > 0) { 
						//	ret = transfer_to_remote(related, fds, nfd, 0); }
						//else {
						//	ret = transfer_to_remote(related, fds, nfd, 1); }

							int flag = 0;
							if(related->remote_host < 0)
								flag = 1;

							if((ret =  transfer_to_remote(related, fds, nfd, flag))== -1) {
								printf("close client\n");
								close(related->client);
								fds[i].fd = -1;
								if(related->remote_host > 0) {
									nfd++;
									for(j = 0; j < nfd; j++) {
                                                                        	if(fds[j].fd == related->remote_host) {
                                                                                	close(fds[j].fd);
                                                                                	fds[j].fd=-1;
                                                                        	}
                                                                	}
								}
								last = remove_conn_info(fds, i, prev, related, last);
							}
							if(ret == 1) {
								//printf("\n close the conn\n");
								//close(related->client);
								//fds[i].fd = -1;
								//last = remove_conn_info(fds, i, prev, related, last);
							}
							if(ret == 2) {
								//fds[i].fd = -1;
                                                                //last = remove_conn_info(fds, i, prev, related, last);
							}
							if(ret == 0) {
								//we added host
								nfd++;
							}
						//}
					}
					else {
						printf("message came for %d (back to cli)\n\n", fds[i].fd);
						if(related->remote_host == fds[i].fd) {
							ret = transfer_back(related);
							if(ret != 2) {
							printf("transfer back succ, socket %d\n", fds[i].fd);
							close(fds[i].fd);
							fds[i].fd=-1;
							related->remote_host = -1;
							related->answer_parsed = 0;
							/*
							for(j = 0; j < nfd; j++) {
                                                                        if(fds[j].fd == related->client) {
                                                                                close(fds[j].fd);
                                                                                fds[j].fd=-1;
                                                                        }
                                                        }
							*/
							//last = remove_conn_info(fds, i, prev, related, last);
							printf("DELETED\n");
							}
						}
					else {
						printf("can't find info about thist desc\n");
					}
					}
			}
		}
		nfd = evacuate_fds(fds);
	}
	for(i = 0; i < nfd; i++) {
		if(fds[i].fd >= 0) 
			close(fds[i].fd);
	}
	return 0;
}
