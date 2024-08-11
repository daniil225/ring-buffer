#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <cmath>
#include <string>
#include <iostream>

int benchmark(char* rb_size, char* freq, char* file_to_save);

int main()
{

    int KB = 1024;
    int MB = KB*1024;


    /* 4KB, 16KB, 32KB, 256KB */
    std::vector<int> small_cach = {4*KB, 16*KB, 32*KB, 256*KB};
    std::vector<int> small_freq = {25, 100, 250, 500, 750, 1000};
    
    /* 512KB, 1024KB, 2048KB, 4096KB */
    std::vector<int> mid_cach = {512*KB, 1024*KB, 2048*KB, 4096*KB};
    std::vector<int> mid_freq = {250, 500, 750, 1000, 2000};
    
    /* 8MB, 16MB, 32MB */
    std::vector<int> big_cach = {8*MB, 16*MB, 32*MB};
    std::vector<int> big_freq = {500, 1000, 1500, 2000, 4000};
    
    
    for(int cach: small_cach)
    {
        for(int freq: small_freq)
        {
            std::string file_for_save = "py/test_res_" + std::to_string(cach/KB) + "KB_" + std::to_string(freq) + ".csv";
            char* rb_size = const_cast<char*>(std::to_string(cach).c_str());
            char* freq_ = const_cast<char*>(std::to_string(freq).c_str());
            
            benchmark(  rb_size, freq_, const_cast<char*>(file_for_save.c_str()));
            sleep(10);
        }

        sleep(60);
    }

    
    for(int cach: mid_cach)
    {
        for(int freq: mid_freq)
        {
            std::string file_for_save = "py/test_res_" + std::to_string(cach/KB) + "KB_" + std::to_string(freq) + ".csv";
            char* rb_size = const_cast<char*>(std::to_string(cach).c_str());
            char* freq_ = const_cast<char*>(std::to_string(freq).c_str());
            
            benchmark(  rb_size, freq_, const_cast<char*>(file_for_save.c_str()));
            sleep(10);
        }

        sleep(60);
    }


    for(int cach: big_cach)
    {
        for(int freq: big_freq)
        {
            std::string file_for_save = "py/test_res_" + std::to_string(cach/MB) + "MB_" + std::to_string(freq) + ".csv";
            char* rb_size = const_cast<char*>(std::to_string(cach).c_str());
            char* freq_ = const_cast<char*>(std::to_string(freq).c_str());
            
            benchmark(  rb_size, freq_, const_cast<char*>(file_for_save.c_str()));
            sleep(10);
        }

        sleep(60);
    }




    
    return 0;
}

int benchmark(char* rb_size, char* freq, char* file_to_save)
{
     pid_t pid_writer, pid_reader;

    pid_writer = fork();
    if(pid_writer == 0)
    {
        /* В первоп процессе */
        char *args[] = {"py/./writer", rb_size, freq , NULL};

        if(execve("py/./writer", args, NULL) == -1)
        {
            fprintf(stderr, "writer error\n");
            exit(1);
        }
    }
    else if(pid_writer > 0)
    {
        /* Находимся в родителе */
        usleep(50000);
        pid_reader = fork();
        if(pid_reader == 0)
        {
            /* Процесс потомок */
            char *args[] = {"py/./reader", rb_size ,file_to_save,NULL};
            if(execve("py/./reader", args, NULL) == -1)
            {
                fprintf(stderr,"reader error\n");
                exit(1);
            }
        }
        else if(pid_reader == -1)
        {
            fprintf(stderr,"error pid reader\n");
            return 1;
        }
        else if(pid_reader > 0)
        {
            /* Находимся в родители обоих процессов */
            sleep(120);

            kill(pid_writer, SIGINT);
            kill(pid_reader, SIGINT);

            int32_t status;
            waitpid(pid_writer, &status, NULL);
            printf("Status writer= %d\n", status);

            waitpid(pid_reader, &status, NULL);
            
            printf("Status reader= %d\n", status);

            printf("--Benchmark done. Buffer size = %s mess per sec = %s", rb_size, freq);
        }
    }
    else if(pid_writer == -1)
    {
        fprintf(stderr,"error pid writer\n");
        return 1;
    }

    return 0;
}