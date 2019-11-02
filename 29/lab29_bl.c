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

#define MAX_HEADER_SIZE 2048
#define MAX_BODY_SIZE 2048
#define FDS_SIZE 200

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
	struct ClientHostList* next;
};


struct CacheUnit {
	struct List* mes_head;
	char* url;
	int waiting_now;
	int is_downloaded;
	int id;
	struct CacheUnits* next;
	//int popularity;
};


struct Cache {
	struct CacheUnit* units_head;
	int max_id;
};

void init_cache_unit(struct CacheUnit* cache_unit, int id, char* url, int url_size) {
 	cache_unit = (struct CacheUnit*)malloc(sizeof(struct CacheUnit));
        cache_unit->id = id;
	cache_unit->NULL;
        cache_unit->waiting_now = 1;
        cache_unit->is_downloading = 1;
	cache_unit->mes_head = (struct List*)malloc(sizeof(struct List));
	cache_unit->mes_head->next=NULL;
	if(url_size != 0) {
		 cache_unit->url = (char*)malloc(sizeof(char)*url_size);
	}
	else {
		 cache_unit->url=NULL;
	}
}

void init_cache(struct Cache* cache) {
	cache = (struct Cache*)malloc(sizeof(struct Cache));
	cache->max_id = 0;
	init_cache_head(cache->units_head, 0);
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
		if(unit->url != NULL)
			free(unit->url);
		prev = unit;
		unit = unit->next;
		free(prev);
	}
}


int divide_body(char* response, struct HttpAnswer* answer) {
	char *tmp;
	char part[MAX_HEADER_SIZE+1];

	tmp = strstr(response, "\r\n\r\n");
	if(tmp != NULL){
		strcpy(answer->body, tmp + strlen("\r\n\r\n"));
		strncpy(answer->header, response, (tmp-response));
		//strcat(answer->header, part);
		//if(tmp < strlen(response))
		//	return 1;
	}
	return 0;
//	printf("head: %s\nbody: %s\n", answer->header, answer->body);
}


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
        //        printf("arg  is: %s\n", ptr);
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
          //      printf("arg  is: %s\n", ptr);
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

//	printf("start parsing\n\n");
	while(ptr != NULL) {
	//	printf("line is: %s\n", ptr);
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
		ptr = strtok(NULL, "\r\n");
	}

	head = head->next;
	while(head != NULL) {
	//	printf("tmp : %s\n", head->str);
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
			 if(remote_answer != NULL)
                                pars_answer_line(head->str, remote_answer, 0);
		}
		cur = head;
		head = head->next;
		free(cur);
	}

	if(response != NULL)
		pars_path(response);
/*
	printf("host - %s\n", response->host);
	printf("path - %s\n", response->path);
	printf("protocol - %s\n", response->protocol);
	printf("method - %s\n", response->method);
*/
}


//pars if we have port
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

//	struct in_addr* adr = ((struct in_addr**)(hp -> h_addr_list))[0];
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

	return sc;
}


void form_http_request(struct HttpParams* response, char* header) {
	// response->protocol;
	sprintf(header, "%s %s %s\r\nHost: %s\r\n\r\n", response->method, response->path, "HTTP/1.0", response->host);
//	printf("header: %s\n", header);
}

struct ClientHostList* find_related(struct ClientHostList* head, int find_him) {
//	printf("find related\n");
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


int transfer_to_remote(struct ClientHostList* related, struct pollfd* fds, int nfd) {
	char buf[MAX_HEADER_SIZE + MAX_BODY_SIZE+1];
	int remote_host, client;
	client = related->client;
	int readen = recv(client, buf, 2048, 0);
        if(readen < 0) {
                printf("error reading\n");
                return -1;
        }
        printf("%s\n\n", buf);

        struct HttpParams* param;
        param = (struct HttpParams*)malloc(sizeof(struct HttpParams));

        parse_request(buf, param, NULL);
        remote_host = get_remote_socket(param->host, "");
        if(remote_host < 0) {
                return -1;
        }
        char* send_this;
        send_this = (char*)malloc(sizeof(char)*3*MAX_HEADER_SIZE);
        printf("form hhtp request\n\n");
        form_http_request(param, send_this);

        printf("send this: %s\n", send_this);
        if(write(remote_host, send_this, strlen(send_this)) < 0) {
                printf("can't write to remote host\n");
                return -1;
        }
	related->remote_host = remote_host;
	fds[nfd].fd = remote_host;
	fds[nfd].events = POLLIN;
	free(param);
}

int hexadecimalToDecimal(char hexVal[]) {

    int len = strlen(hexVal);
    int base = 1, i;
    int dec_val = 0;

    for (i = len-1; i>=0; i--) {
        if (hexVal[i]>='0' && hexVal[i]<='9') {
            dec_val += (hexVal[i] - 48)*base;
            base = base * 16;
	}
       else if (hexVal[i]>='a' && hexVal[i]<='f') {
            dec_val += (hexVal[i] - 87)*base;
            base = base*16;
        }
    }

    return dec_val;
}

/*
int transfer_back(struct ClientHostList* related) {
	printf("transfer back\n");

	char buf[MAX_HEADER_SIZE + MAX_BODY_SIZE+1] = "";
	int remote_host = related->remote_host;
	int client = related->client, flag = 0,  mes_len, is_enough = 0;
	int try_read = 0, readen;
	struct HttpAnswer* ans;
        ans = (struct HttpAnswer*)malloc(sizeof(struct HttpAnswer));
	(ans->body)[0] = '\0';

	//find empty str
	while(strlen(ans->body) == 0 && is_enough == 0) {
	printf("continue reading head\n\n======================\n\n");
        readen = read(remote_host, buf, MAX_HEADER_SIZE+MAX_BODY_SIZE);
        buf[readen+1] = '\0';

	printf("got in head part:\n%s\n\n", buf);
	if(readen < 0) {
                printf("error reading from remote host");
                return -1;
        }

        if(write(client, buf, strlen(buf)) < 0) {
                printf("can't write to client - %d\n\n", client);
                return -1;
        }

        ans->chunked = 0;
	ans->content_lengh=-1;
	if(divide_body(buf, ans) == 1)
		is_enough
	printf("---------------------\nbody is: %s\n==============\n\n", ans->body);

        try_read+=strlen(ans->body);
        parse_request(ans->header, NULL, ans);
//	printf("readen now : %d of %d\n\n", try_read, ans->content_lengh);

	mes_len =  ans->content_lengh;
	int dec_number = 0;
	if((ans->chunked) == 1) {
		char number[1024] = "";
		sscanf(ans->body, "%s\n", number);
		dec_number = hexadecimalToDecimal(number);
		printf("CHUNKED, first time : %s\n", number);
		if(strcmp(number, "") == 0) {
			flag = 1;
		}
		else {
			mes_len = dec_number;
		}
	}
//	printf("TIME to READ the rest\n\n");
	//if we haven't readen all head
	}
	printf("TIME to READ the rest\n\n");
        while(flag == 1 || try_read < mes_len) {
		//char buf1[MAX_HEADER_SIZE + MAX_BODY_SIZE+1] = "";
		readen = read(remote_host, buf, MAX_HEADER_SIZE+MAX_BODY_SIZE);
                buf[readen+1]='\0';
		if(readen < 0) {
                        printf("error reading from remote host-additional read\n");
                        return -1;
                }
                if(readen == 0)
                        break;
                try_read += readen;
		if(write(client, buf, strlen(buf)) < 0) {
                	printf("can't write to remote host\n");
        	        return -1;
	        }
                printf("\n\nagain readen:\n\n%s", buf);
        }

	if(ans->chunked == 1 && mes_len != 0) {
		free(ans);
		return 2;
	}
	free(ans);
	return 0;
}
*/


int transfer_back(struct ClientHostList* related) {
	char buf[MAX_HEADER_SIZE + MAX_BODY_SIZE+1] = "";
	int remote_host = related->remote_host;
	int client = related->client;
	int try_read = 0, readen;
        //copy buf??
        readen = read(remote_host, buf, MAX_HEADER_SIZE+MAX_BODY_SIZE);
	 buf[readen+1] = '\0';

	if(readen < 0) {
                printf("error reading from remote host");
                return -1;
        }

	if(readen == 0) { 
		printf("\n now it's null\n");
		return 1;
	}
	buf[7] = '0';
	printf("got:\n%s\n\n", buf);

        if(write(client, buf, strlen(buf)) < 0) {
                printf("can't write to remote host\n");
                return -1;
        }
        struct HttpAnswer* ans;
        ans = (struct HttpAnswer*)malloc(sizeof(struct HttpAnswer));
        (ans->body)[0] = '\0';
	ans->chunked = 0;
	ans->content_lengh = -1;
	divide_body(buf, ans);

        try_read+=strlen(ans->body);
//      printf("readen bytes - %d and body - %d\n", readen, strlen(ans->body));
        parse_request(ans->header, NULL, ans);
//      printf("content-len: %d answer %s\n", ans->content_lengh, ans->status);

	printf("divided, chunced=%d\n", ans->chunked);
	printf("readen now : %d of %d\n\n", try_read, ans->content_lengh);

	int mes_len =  ans->content_lengh, flag = 0, dec_number = 0;
/*	if((ans->chunked) == 1) {
		char number[1024] = "";
		sscanf(ans->body, "%s\n", number);
		dec_number = hexadecimalToDecimal(number);
		printf("CHUNKED, first time : %s\n", number);
		if(strcmp(number, "") == 0) {
			flag = 1;
		}
		else {
			mes_len = dec_number;
		}
	}
*/

        while(try_read < mes_len) {
		//char buf1[MAX_HEADER_SIZE + MAX_BODY_SIZE+1] = "";
		readen = read(remote_host, buf, MAX_HEADER_SIZE+MAX_BODY_SIZE);
                buf[readen+1]='\0';
		if(readen < 0) {
                        printf("error reading from remote host-additional read\n");
                        return -1;
                }
                if(readen == 0)
                        break;
                try_read += readen;
		/*
		if(flag == 1) {
			char number[1024] = "";
			sscanf(buf, "%s\n", number);
			dec_number = hexadecimalToDecimal(number);
        	        printf("CHUNKED : hex-%s, dec-%d\n", number, dec_number);
			flag = 0;
			mes_len = dec_number;
		}
		*/
		if(write(client, buf, strlen(buf)) < 0) {
                	printf("can't write to remote host\n");
        	        return -1;
	        }
                printf("again readen:\n\n%s", buf);
        }

	if(mes_len < 0) {
		free(ans);
		return 2;
	}
	free(ans);
	return 0;
}


struct ClientHostList* remove_conn_info(struct pollfd* fds, int i, struct ClientHostList* prev, struct ClientHostList* related,  struct ClientHostList* last) {
	//if(fds[i].fd >= 0)
        //      close(fds[i].fd);
        //fds[i].fd = -1;
	printf("remove_conn_info\n");
	printf("prev cli %d, host %d\n", prev->client, prev->remote_host);
        prev->next = related->next;
//        free(related);
	if(prev->next == NULL)
		return prev;
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

int main(int argc, char* argv[]) {
	int i, sc, client, remote_host, ret;
	struct sockaddr_in sc_addr;
//	char buf[MAX_HEADER_SIZE + MAX_BODY_SIZE];
	int addrlen = sizeof(sc_addr);

//	sc = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	sc = socket(AF_INET, SOCK_STREAM, 0);
	if(sc == -1) {
		close(sc);
		printf("can't create socket\n");
		return 1;
	}

	int opt = 1;
	//SO_REUSEADDR,  TCP_NODELAY
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

	//printf("binded: %d\n", sc);
	if(listen(sc, SOMAXCONN) < 0) {
		printf("errir listen\n");
		return -1;
	}

	//---
	//size??
	struct pollfd fds[FDS_SIZE];
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
	//	printf("try poll\n");
		ret = poll(fds, nfd, timeout);
		printf("poll!!\n");
		if(ret < 0) {
			printf("poll failed\n");
			exit(-1);
		}
		if(ret == 0) {
			printf("exit after timeout\n");
			exit(1);
		}
		size_copy = nfd;
	//	printf("size_copy %d\n", size_copy);
		for(i = 0; i < size_copy; i++) {
			//char buf[MAX_HEADER_SIZE + MAX_BODY_SIZE];
			if(fds[i].revents == 0) {
			//	printf("events = 0\n");
				continue;
			}
			//delete this fd
			if(fds[i].revents != POLLIN) {
			//	 printf("events not pollin\n");
				continue;
			}
			//printf("Oh\n");
			if(fds[i].fd == sc) {
				//it's me
				client = 0;
				//while(client != -1) { 
					//printf("try accept self=%d, socket-to-acc=%d\n", sc, fds[i].fd);
					if((client = accept(fds[i].fd, (struct sockaddr*)&sc_addr, (socklen_t*)&addrlen)) < 0) {
                				//printf("error accept\n");
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
					}

					struct ClientHostList* new_client;
					new_client = (struct ClientHostList*)malloc(sizeof(struct ClientHostList));
					new_client->client = client;
					new_client->remote_host = -1;
					new_client->next=NULL;
					last->next = new_client;
					last = new_client;
					
					for(j = 0; j < nfd; j++) {
						printf("fd: %d\n", fds[j].fd);
					}
					printf("accepting sucess %d, nfd=%d\n", client, nfd);
				//}
			}
			else {
					if(head == NULL)
						printf("head is null\n");
					struct ClientHostList* prev = find_related(head, fds[i].fd);
					if(prev == NULL)
						printf("prev is null\n");
					struct ClientHostList* related = prev->next;

					if(related->client == fds[i].fd) {
						 printf("message came for %d (to host), and host is %d, client %d\n\n", fds[i].fd, related->remote_host, related->client);
						if(related->remote_host > 0) {
                                                        char buf[4];
                                                        if ((recv(related->remote_host, buf, sizeof(buf), MSG_PEEK | MSG_DONTWAIT)) == -1) {
                                                                printf("should close\n");
								//close(fds[i].fd);
								fds[i].fd = -1;
								/*for(j = 0; j < nfd; j++) {
									if(fds[j].fd == related->remote_host) {
										close(fds[j].fd);
										fds[j].fd=-1;
									}
								}
								*/
                                                        	last = remove_conn_info(fds, i, prev, related, last);
							}
                                                }
						else {
						if(transfer_to_remote(related, fds, nfd) == -1) {
							//remove from fds and list
							//shift all
							last = remove_conn_info(fds, i, prev, related, last);
						}
						else {nfd++;}
						}
					}
					else {
						  printf("message came for %d (back to cli)\n\n", fds[i].fd);
						if(related->remote_host == fds[i].fd) {
							if(transfer_back(related) != 2) {

							printf("transfer back succ, socket %d\n", fds[i].fd);
							close(fds[i].fd);
							fds[i].fd=-1;
							for(j = 0; j < nfd; j++) {
                                                                        if(fds[j].fd == related->client) {
                                                                                close(fds[j].fd);
                                                                                fds[j].fd=-1;
                                                                        }
                                                                }

							//struct ClientHostList* cur2 = head;
							//while(cur2 != NULL) {
							//	printf("cli %d, host %d\n", cur2->client, cur2->remote_host);
							//	cur2 = cur2->next;
							//}
 							// printf("last cli %d, host %d\n\n", last->client, last->remote_host);
							last = remove_conn_info(fds, i, prev, related, last);
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
		for(i = 0; i < nfd; i++)
			printf("fds: %d, ", fds[i]);
		printf("\n\n");
	}
	for(i = 0; i < nfd; i++) {
		if(fds[i].fd >= 0) 
			close(fds[i].fd);
	}
	return 0;
}
