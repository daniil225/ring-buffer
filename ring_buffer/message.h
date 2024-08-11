#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <stdint.h>
#include <stdio.h>

typedef struct _message
{
    int64_t sec;
    int64_t nsec;
    uint64_t mess_id;
    size_t buffer_size;

    char buffer[512];
}message_t;

#endif 