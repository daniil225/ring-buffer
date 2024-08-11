#include "ring_buffer.h"
#include "message.h"

#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>


#define CPU_ID 1

static int32_t measurement_cnt = 0; // Количество полученных измерений 
typedef struct _measurement
{
    uint64_t send_sec;
    uint64_t send_nsec;

    uint64_t retrive_sec;
    uint64_t retrive_nsec;
}measurement_t;

static volatile int32_t is_run = 1;
static void read_message(ring_buffer_t *rb, measurement_t *measurement_buffer);
static void handler(int32_t param);



int main(int32_t argc, char** argv)
{
    int64_t rb_size;
    const char* filepath_param = NULL;
    if (argc != 3) 
    {
        printf("Usage: %s SIZE PATH\n", argv[0]);
        printf("  where SIZE is size of ring buffer in bytes.\n");
        printf("  where PATH is the path for save the measurement in csv format");
        return -1;
    }

    rb_size = atoll(argv[1]);
    filepath_param = argv[2];

    /* Обработчик прервывания */
    signal(SIGINT, &handler);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(CPU_ID, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) 
    {
        fprintf(stderr, "pthread_setaffinity_np faield\n");
        return -1;
    }

    ring_buffer_t rb;
    
    int32_t init_rb_code = init_ring_buffer(&rb,rb_size, SHM_MEM_ID, RING_BUFFER_GET);
   
    if(init_rb_code != 0)
    {
        fprintf(stderr, "can not init ring_buffer_t.\nReturn code = %d\n", init_rb_code);
        return -2;
    }

    /* Если буфер смогли получить : */
    printf("Reader work start...\n");
    printf("ring_buffer_t create with param:\n  buffer size = %ld\n  fd = %d\n  shm_id = %s\n", rb.rb_impl->buffer_size, rb.rb_impl->fd, rb.rb_impl->shm_mem_id);
   
    measurement_t *measurement_buffer = (measurement_t*)malloc(sizeof(measurement_t)*128*1024*1024);

    while(is_run) read_message(&rb, measurement_buffer);

    /* Сохраняем результаты в файл */
    char filepath[256];
    strncpy(filepath,filepath_param, sizeof(filepath)-1);

    printf("filepath = %s\n", filepath);
    
    FILE* results = fopen(filepath, "w");
    fprintf(results, "send_sec,send_nsec,retrieve_sec,retrieve_nsec,\n");
    for (size_t i = 0; i < measurement_cnt; i++) 
    {
        fprintf(results, "%lu,%lu,%lu,%lu,\n",
	    measurement_buffer[i].send_sec, measurement_buffer[i].send_nsec, measurement_buffer[i].retrive_sec, measurement_buffer[i].retrive_nsec);
    }
    printf("Saved measure results to '%s'\n", filepath);
    fclose(results);
    destroy_ring_buffer(&rb);
    free(measurement_buffer);

    return 0;
}

static void read_message(ring_buffer_t *rb, measurement_t *measurement_buffer)
{
    size_t size;
    message_t *mess = read_from_ring_buffer(rb, &size);

    //printf("read size = %lu\n", size);
    while(size > 0)
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        /* Получаем данные */
        measurement_buffer[measurement_cnt++] = (measurement_t)
        {
            .send_sec = mess->sec,
            .send_nsec = mess->nsec,
            .retrive_sec = ts.tv_sec,
            .retrive_nsec = ts.tv_nsec
        };

        size_t mess_size_byte = sizeof(mess->mess_id) + sizeof(mess->nsec) + sizeof(mess->sec) + mess->buffer_size;
        mess = (message_t*)((uint8_t*)mess + mess_size_byte);
        size -= mess_size_byte;   
    }   
}


static void handler(int32_t param)
{
    printf("reader proc interrupt\n");
    is_run = 0;
}