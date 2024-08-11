#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#define _GNU_SOURCE  // Для того что бы MAP_ANONYMOUS было доступно 
#include <stdint.h>
#include <stdio.h>


#define SHM_MEM_ID "/my_shm_mem1"

enum init_mode_ring_buffer_t
{
    RING_BUFFER_INIT = 1,
    RING_BUFFER_GET = 2
};

/* Данные буфера. Удобно разделить, что бы произвести мапинг по виртуальной памяти  */
typedef struct _ring_bufer_data
{
    int32_t fd;
    size_t head;
    size_t tail;
    size_t buffer_size;
    char shm_mem_id[64]; // Идентификатор разделаемой памяти  

    _Atomic uint32_t mode; // переменная регулирует доступ для чтения или записи. mode %2 == 0 можно читать, mode % 2 == 1 читать нельзя идет запись. 
    uint32_t ref_cnt; // Количество ссылок на буфер
    /* data */
}ring_bufer_data_t;

/* Кольцевой буфер */
typedef struct _ring_buffer
{
    uint8_t *buffer;

    ring_bufer_data_t *rb_impl;

}ring_buffer_t;




/*
    @brief Инициализирует кольцевой буфер 
    @param ring_buffer_t *rb - Структура для инициализации 
    @param size_t rb_size    - Размер кольцевого буфера. Должен быть кратен размеру страницы виртуальной памяти
    @param  char* shm_mem_id - Идентификатор для создания разделаемого сегмета памяти. msx_size = 64 
    @param mode              - Режим инициализации. Либо это создание нового буфера, или это получение уже существующего
    @return int32_t          - Коды возвратов:
                            0 - OK 
                            1 - _Atomic не достуен
                            2 - rb_size % page_size != 0 - размер буфера не кратен размеру страницы виртуальной памяти 
                            3 - Не удалось создать разделаемую память 
                            4 - ftruncate не смог сделать ресайз для разделаемого сегмента памяти 
                            5 - mmap не смог отобразить память в виртуальное пространство адресов 
                            6 - Некорретный размер для получения размера уже инициализированного буфера
                            7 - Не совпадающее имя идентификатора разделаемой области памяти
*/
int32_t init_ring_buffer(ring_buffer_t* rb, size_t rb_size, char* shm_mem_id, enum init_mode_ring_buffer_t mode);

/*
    @brief Деинициализация кольцевого буфера. Производим очистку всех ресурсов. 
    @param ring_buffer_t *rb - Структура кольцевого буфера 
    @return int32_t          - Коды возвратов 
                            0 - OK
                            1 - munmap не смог выполнить отсоединение памяти и провести очистку ресурсов 
                            2 - shm_unlink не смог отсоеденить разделаемую пямть от дескриптора
*/
int32_t destroy_ring_buffer(ring_buffer_t* rb);


/*
    @brief Чтение из кольцевого буфера
    @param ring_buffer_t *rb - Структура для инициализации 
    @param size_t* size      - Размер прочитанного сообщения в байтах 
    @return void*            - указатель на данные, которые прочитали из буфера
*/
void* read_from_ring_buffer(ring_buffer_t* rb, size_t* size);

/*
    @brief Запись в кольцевой буфер
    @param ring_buffer_t *rb - Структура для инициализации
    @param size_t size       - Размер записываемого сообщения в байтах 
    @param const void* mes   - Указатель на буфер сообщения  
    @return int32_t          - Коды возвратов: 
                            0 - OK
                            1 - size > availebel buffer_size - длина сообщения не может быть больше чем доступно для записи 
*/
int32_t write_to_ring_buffer(ring_buffer_t* rb, size_t size, const void* mes);




#endif