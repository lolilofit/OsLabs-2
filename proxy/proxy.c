
#include <errno.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include<signal.h>
#include <fcntl.h>
#include <sys/file.h>

#include "parser.h"
#include "cache.h"

extern int errno;
struct Cache* cache;


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

int transfer_to_remote(struct ClientHostList* related, struct pollfd* fds, int nfd) {

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
        fds[nfd].fd = remote_host;
        fds[nfd].events = POLLIN;
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
                        printf("error reading from remote host-additional read, remote_host\n");
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

        return 1;
}


struct ClientHostList* remove_conn_info(struct pollfd* fds, int i, struct ClientHostList* prev, struct ClientHostList* related, struct ClientHostList* last) {
        if(related != NULL && prev != NULL) {
                prev->next = related->next;
                if(related->cache_unit == NULL)
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
        int on = 1;
        if(ioctl(sc, FIONBIO, (char *)&on) < 0) {
                printf("can't make nonblocking");
                exit(-1);
        }

        sc_addr.sin_family = AF_INET;
        sc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        sc_addr.sin_port = htons(5000);
        if(bind(sc, (struct sockaddr*)&sc_addr, sizeof(sc_addr)) < 0) {
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
		 size_copy = nfd;
                for(i = 0; i < size_copy; i++) {
                        if(fds[i].revents == 0) {
                                continue;
                        }
                        if(fds[i].revents != POLLIN) {
                                continue;
                        }
                        if(fds[i].fd == sc) {
                                client = 0;
                                while(client != -1) {
                                        if((client = accept(fds[i].fd, (struct sockaddr*)&sc_addr,  (socklen_t*)&addrlen)) < 0) {
                                                printf("error accept\n");
                                                continue;
                                        }
                                        printf("Client %d accepted\n\n", client);
                                        fds[nfd].fd = client;
                                        fds[nfd].events = POLLIN;
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
                                        if(head == NULL) {
                                                printf("head is null, should be allocated\n");
                                                continue;
                                        }
                                        struct ClientHostList* prev = find_related(head, fds[i].fd);
                                        if(prev == NULL) {
                                                printf("prev is null\n");
                                                fds[i].fd = -1;
                                                continue;
                                        }
                                        struct ClientHostList* related = prev->next;

                                        if(related->client == fds[i].fd) {
                                                printf("message came for %d (to host), and host is %d, client %d\n\n", fds[i].fd, related->remote_host, related->client);
                                                if((ret =  transfer_to_remote(related, fds, nfd))== -1) {
                                                        printf("close client\n");
                                                        close(related->client);
                                                        fds[i].fd = -1;
                                                        if(related->remote_host > 0) {
							for(j = 0; j < nfd; j++) {
                                                                        if(fds[j].fd == related->remote_host) {
                                                                                close(fds[j].fd);
                                                                                fds[j].fd=-1;
                                                                        }
                                                                }
                                                        }
                                                        last = remove_conn_info(fds, i, prev, related, last);
                                                }
                                                if(ret == 0) {
                                                        //we added host
                                                        nfd++;
                                                }
                                        }
                                        else {
                                        //      printf("message came for %d (back to cli)\n\n", fds[i].f$
                                                if(related->remote_host == fds[i].fd) {
                                                        ret = transfer_back(related);
                                                        if(ret != 2) {
                                                        close(fds[i].fd);
                                                        fds[i].fd=-1;
                                                        related->remote_host = -1;
                                                        printf("host conn deleted\n");
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



