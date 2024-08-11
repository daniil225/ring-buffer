#include "ring_buffer.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int32_t init_ring_buffer(ring_buffer_t *rb, size_t rb_size, char *shm_mem_id, enum init_mode_ring_buffer_t mode)
{
   
#ifdef __STDC_NO_ATOMICS__
    fprintf(stderr, "Not suport _Atomic");
    return 1; // _Atomic не доступен
#endif

    const int32_t page_size = getpagesize();

    if (rb_size % page_size != 0)
    {
        fprintf(stderr, "rb_size = %ld can not be used. It should be divided into page_size = %d\n", rb_size, page_size);
        return 2;
    }

    int32_t fd = shm_open(shm_mem_id, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR); // Делаем память

    if (fd == -1)
    {
        fprintf(stderr, "shm_open error");
        return 3;
    }
 
    /* Посчитаем сколько нам нужно всего байт в страницах  */

    const size_t rb_data_size = ((sizeof(ring_bufer_data_t) + page_size - 1) / page_size) * page_size;

    /* Делаем ресайз для файла разделаемой памяти */
    if(mode == RING_BUFFER_INIT)
    {
        if (ftruncate(fd, rb_data_size + rb_size) == -1)
        {
            fprintf(stderr, "ftruncate can not resize fd = %d to length = %ld\n", fd, rb_data_size + rb_size);
            return 4;
        }
    }

    /*
        А теперь нужно произвести паминг памяти при помощи mmap из следующих соображений:
        1) Начальный адрес вируальной памяти назначим для структуры ring_bufer_data_t, так будет удобно потом производить разметку.
        2) Разметку будем производить для дескритора fd - разделаемая память между процессами
        Общая идея более подробно здесь: https://habr.com/ru/companies/otus/articles/557310/
    */

    /*

        Размер виртуалььной памяти будет rb_data_size + 2*rb_size - собственно это нужно для того, что бы организовать быстрый доступ к памяти
        PROT_NONE     - доступ к этой области памяти запрещен
        MAP_SHARED    - Разделить использование этого отражения с другими процессами, отражающими тот же объект. Запись информации в эту область памяти будет эквивалентна записи в файл.
        MAP_ANONYMOUS - Отображение не резервируется ни в каком файле; аргументы fd и offset игнорируются. Этот флаг вместе с MAP_SHARED реализован с Linux 2.4.
    */
    ring_bufer_data_t *rb_data = mmap(NULL, rb_data_size + 2 * rb_size, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, fd, 0);

    if (rb_data == MAP_FAILED)
    {
        fprintf(stderr, "mmap can not emplace fd to virtual memory\n");
        return 5;
    }

    /* Сохраним смещение в виртуальной памяти для буфера */
    uint8_t *buffer = (uint8_t *)rb_data + rb_size;

    /*
        rb_data - Стратовый адрес в виртуальной памяти
        rb_data_size + rb_size - количество байтов для отображения
        PROT_READ | PROT_WRITE - чтение и запись
        MAP_SHARED - Разделить использование этого отражения с другими процессами, отражающими тот же объект. Запись информации в эту область памяти будет эквивалентна записи в файл.
        MAP_FIXED - Не использовать другой адрес, если адрес задан в параметрах функции. Если заданный адрес не может быть использован, то функция mmap вернет сообщение об ошибке.
                    Если используется MAP_FIXED, то start должен быть пропорционален размеру страницы. Использование этой опции не рекомендуется.(А мы вот будем, но аккуратно)
        fd - файловы дескриптор, который отображаем
        0  - смещение от начала файла

    */
    mmap(rb_data, rb_data_size + rb_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);

    /*
        buffer + rb_size - сместились на позицию буфера 
        rb_size - размер буфера 
        дальше повтор 
        rb_data_size - размер смещения для 

    */
    mmap(buffer + rb_data_size, rb_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, rb_data_size);

    
    /* Инициализируем поля ring_bufer_data_t */
    if(mode == RING_BUFFER_INIT)
    {
        rb_data->fd = fd;
        rb_data->buffer_size = rb_size;
        rb_data->head = 0;
        rb_data->tail = 0;
        rb_data->mode = 0; // Изначально можно читать 
        strncpy(rb_data->shm_mem_id, shm_mem_id, sizeof(rb_data->shm_mem_id)-1);
        rb_data->ref_cnt = 1; // Установили количество ссылок
    } 
    else if(mode == RING_BUFFER_GET)
    { 
        rb_data->ref_cnt ++; 

        printf("Get buffer size = %ld\n", rb_size);
        if (rb_size != rb_data->buffer_size) 
        {
            fprintf(stderr, "invalid size passed (expected %lu, got %lu)\n", rb_data->buffer_size, rb_size);
            ring_buffer_t loc_rb = (ring_buffer_t){.buffer = buffer, .rb_impl = rb_data};
            /* 
                Здесь есть один недостаток, в случае передачи разных идентификаторов разделаемой памяти, та что будет выделена для режима
                RING_BUFFER_GET останется не очищенной и будет занимать ресурс до момента завершения процесса. Явная утечка ресурсов. 
                
                Такую ситацию можно пофиксить при помощи изначчального сравнения уже имеющейся, та что была создана в режиме RING_BUFFER_INIT и 
                той, что была передана в функцию для режима RING_BUFFER_GET. 
                
                !!!Пока данная возможность не реализована  
            */
            destroy_ring_buffer(&loc_rb);
            return 6;
         }
         
    }

    *rb = (ring_buffer_t){.buffer = buffer, .rb_impl = rb_data};

    return 0;
}



int32_t destroy_ring_buffer(ring_buffer_t *rb)
{

    size_t ref_cnt = --rb->rb_impl->ref_cnt;

    if(ref_cnt == 0)
    {

        char shm_mem_id_cpy[64];
        strcpy(shm_mem_id_cpy, rb->rb_impl->shm_mem_id);
        //shm_mem_id_cpy[strlen(rb->rb_impl->shm_mem_id)] = '\0';    

        const int32_t page_size = getpagesize();
        const size_t rb_data_size = ((sizeof(ring_bufer_data_t) + page_size) / page_size) * page_size;


        if(munmap(rb->rb_impl, rb->rb_impl->buffer_size + rb_data_size) == -1)
        {
            fprintf(stderr, "munmap error\n");
            return 1;
        }

        if(shm_unlink(shm_mem_id_cpy) == -1)
        {
            fprintf(stderr, "shm_unlink can not unlink %s memory region\n", shm_mem_id_cpy);
            return 2;
        }
    }


    return 0;

}

void* read_from_ring_buffer(ring_buffer_t *rb, size_t *size)
{

    uint8_t* buffer = rb->buffer;
    ring_bufer_data_t* rb_data = rb->rb_impl;

    /* 
        Провераем можно ли читать из буфера 
        Если rb_data->mode % 2 == 0 - можно 
        Если rb_data->mode % 2 == 1 - нельзя идет запись 
    */
    void* data = NULL;

    size_t mode = rb_data->mode;
    while (mode & 1){ mode = rb_data->mode; }

    *size = rb_data->tail - rb_data->head; 
    data = &buffer[rb_data->head];
    
    rb_data->head = rb_data->tail;

    if(rb_data->head > rb_data->buffer_size)
    {
        rb_data->head -= rb_data->buffer_size;
        rb_data->tail -= rb_data->buffer_size;  
    }
    return data;
}

int32_t write_to_ring_buffer(ring_buffer_t *rb, size_t size, const void *mes)
{
    uint8_t* buffer = rb->buffer;
    ring_bufer_data_t* rb_data = rb->rb_impl;

    /* Первая проверка на то, что входной буфер не больше чем доступно памяти */
    if(rb_data->buffer_size - (rb_data->tail - rb_data->head) < size)
    {
        fprintf(stderr,"mesage size = %ld can not more than availebel buffer size = %ld\n", size, rb_data->buffer_size- (rb_data->tail - rb_data->head));
        return 1;
    }

    /* Запрещаем чтение */
    rb_data->mode++;
    
    /* Пишем в буфер */
    memcpy(&buffer[rb_data->tail],mes,size);
    rb_data->tail += size;

    /* Разрешаем чтение */
    rb_data->mode++;

    return 0;
}
