#ifndef CACHE_H
#define CACHE_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include <fcntl.h>
#include <sys/file.h>
#include"definings.h"
#include"list.h"

struct ClientHostList {
        int client;
        int remote_host;
        int should_cache;
        char* url;
        struct ClientHostList* next;
        struct CacheUnit* cache_unit;
};


struct CacheUnit {
        struct List* mes_head;
        struct List* last_mes;
        char* url;
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

        unit->last_mes->next = add_this;
        unit->last_mes = unit->last_mes->next;
}


struct CacheUnit* find_cache_by_url(struct Cache* cache, char* url) {
        printf("find in cache by url : %s, size %d\n", url, strlen(url));
        struct CacheUnit* cur = cache->units_head->next;
        while(cur != NULL) {
                if(strcmp(url, cur->url) == 0) {
                        printf("FOUND in cache\n");
                        return cur;
                }
                cur = cur->next;
        }
        printf("not found\n");
        return NULL;
}


struct CacheUnit* init_cache_unit(int id, char* url) {
        struct CacheUnit* cache_unit;
        cache_unit = (struct CacheUnit*)malloc(sizeof(struct CacheUnit));
        cache_unit->next = NULL;
        cache_unit->mes_head = (struct List*)malloc(sizeof(struct List));
        cache_unit->mes_head->next=NULL;
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
        cache->units_head = init_cache_unit(0, head_mes);
        return cache;
}


struct CacheUnit* add_cache_unit(struct Cache* cache, int id, char* url) {
        struct CacheUnit* cache_unit = NULL;
        struct CacheUnit* cur =  cache->units_head->next;
        if(cur == NULL) {
                        cache_unit = init_cache_unit(id, url);
                        cache->units_head->next = cache_unit;
                        return cache_unit;
        }

        while(cur != NULL) {
                if(strcmp(cur->url, url) == 0)
                        return NULL;
                if(cur->next == NULL) {
                        cache_unit = init_cache_unit(id, url);
                        cur->next = cache_unit;
                        return cache_unit;
                }
                cur = cur->next;
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
                prev = unit;
                unit = unit->next;
                free(prev);
        }
}


#endif
