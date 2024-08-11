#include "ring_buffer.h"
#include "message.h"

#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>

#define CPU_ID 0

static volatile int32_t is_run = 1;
static void send_message(ring_buffer_t *rb);
static void handler(int32_t param);


int main(int32_t argc, char **argv)
{
    srand(time(NULL));
    
    int64_t rb_size, mes_frequency;

    if (argc != 3) 
    {
        printf("Usage: %s SIZE FREQ\n", argv[0]);
        printf("SIZE: is size of ring buffer in bytes.\n");
        printf("FREQ: is number of messages send in second.\n");
        return -1;
    }
    rb_size = atoll(argv[1]);
    mes_frequency = atoll(argv[2]);
        
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
   

    int32_t init_rb_code = init_ring_buffer(&rb,rb_size, SHM_MEM_ID, RING_BUFFER_INIT);

   
    
    if(init_rb_code != 0)
    {
        fprintf(stderr, "can not init ring_buffer_t.\nReturn code = %d\n", init_rb_code);
        return -2;
    }

    
    /* Если буфер смогли создать: */
    printf("Writer start...\n");
    printf("ring_buffer_t create with param:\n  buffer size = %ld\n  fd = %d\n  shm_id = %s\n", rb.rb_impl->buffer_size, rb.rb_impl->fd, rb.rb_impl->shm_mem_id);
   
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1e9/mes_frequency;

    while(is_run)
    {
        nanosleep(&ts, NULL);
        send_message(&rb);
       
    }

    destroy_ring_buffer(&rb);

    return 0;
}


static void send_message(ring_buffer_t *rb)
{
    char* symdols = "abcdefghijklmnopqrstyvwxyz1234567890";
    const size_t symdols_len = strlen(symdols);
    static int32_t mess_counter = 0;
    struct timespec ts;
    message_t mess;
    mess.buffer_size = rand() % symdols_len + 10;
    const size_t mess_size = sizeof(mess.mess_id) + sizeof(mess.nsec) + sizeof(mess.sec) + mess.buffer_size;

    for(int32_t i = 0; i < mess.buffer_size; i++) mess.buffer[i] = symdols[rand() % symdols_len];
    mess.mess_id = mess_counter;
    mess_counter++;

    /* Записываем время */
    clock_gettime(CLOCK_REALTIME, &ts);
    mess.sec = ts.tv_sec;
    mess.nsec = ts.tv_nsec;

    write_to_ring_buffer(rb, mess_size, &mess);
    //printf("mess write = %d\n", mess_counter);
}

static void handler(int32_t param)
{
    printf("witer proc interrupt\n");
    is_run = 0;
}