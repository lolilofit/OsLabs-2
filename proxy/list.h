#ifndef LIST_H
#define LIST_H

struct List {
        char str[MAX_HEADER_SIZE+MAX_BODY_SIZE+1];
        int len;
        struct List* next;
};

#endif
