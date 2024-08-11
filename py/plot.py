import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import pathlib

# определение пути
currentDirectory = pathlib.Path('.')
	 





def sort_data_result(data_result):
    res = []
    key_val = []

    for data in data_result:
        data_str = str(data).split("_")
        freq = data_str[3].split(".")[0]
        key_val.append((int(freq), data))
    
    key_val.sort(key=lambda x: x[0])
    data_result = []

    for el in key_val: 
        res.append(el[1])

    return res


def gen_table_meserment(buffer_size):
    currentPattern = "test_res_"+ buffer_size +"_*.csv"
    data_result = []
    
    for currentFile in currentDirectory.glob(currentPattern):  
	    data_result.append(currentFile)



    data_result = sort_data_result(data_result)

    teable_str = "|"+ str(buffer_size)
    for data in data_result:

        data_str = str(data).split("_")
        size = data_str[2]
        freq = data_str[3].split(".")[0]

        teable_str+=  "|![](py/img/test_res_" + size + "_" + freq + ".png)"
    teable_str += "|"

    return teable_str


def print_p99_table(buffer_size):
    
    currentPattern = "test_res_"+ buffer_size +"_*.csv"
    data_result = []
    
    for currentFile in currentDirectory.glob(currentPattern):  
	    data_result.append(currentFile)
         
    data_result = sort_data_result(data_result)


    res_str = "|" + str(buff_sz)

    for data in data_result:

        df = pd.read_csv(data)

        data_str = str(data).split("_")
        size = data_str[2]
        freq = data_str[3].split(".")[0]

        send_sec = df['send_sec']
        send_sec -= send_sec.min()

        retrieve_sec = df['send_sec']
        retrieve_sec -= retrieve_sec.min()

        send = send_sec + df['send_nsec']/(10**9)
        retrieve = retrieve_sec + df['retrieve_nsec']/(10**9)

        ind = np.searchsorted(send, 20.0)
        send = send[ind:]
        retrieve = retrieve[ind:]


        diff = np.clip(retrieve-send, 0, 1)
    
        p99 = np.percentile(diff, 99)

        p99 = 1000000*p99
   

        res_str += "|" + "{:.2f}мкс".format(p99)

    res_str += "|"
    return res_str


def save_and_plot_res(buffer_size):
    
    # Параметры возврата для дальнейшего анализа
    size = ""
    procentile = []
    mes_per_sec = []

    currentPattern = "test_res_"+ buffer_size +"_*.csv"
    data_result = []
    
    for currentFile in currentDirectory.glob(currentPattern):  
	    data_result.append(currentFile)



    data_result = sort_data_result(data_result)
    print(data_result)

    for data in data_result:

        df = pd.read_csv(data)

        data_str = str(data).split("_")
        size = data_str[2]
        freq = data_str[3].split(".")[0]

        send_sec = df['send_sec']
        send_sec -= send_sec.min()

        retrieve_sec = df['send_sec']
        retrieve_sec -= retrieve_sec.min()

        send = send_sec + df['send_nsec']/(10**9)
        retrieve = retrieve_sec + df['retrieve_nsec']/(10**9)

        ind = np.searchsorted(send, 20.0)
        send = send[ind:]
        retrieve = retrieve[ind:]


        diff = np.clip(retrieve-send, 0, 1)
    
        p99 = np.percentile(diff, 99)

        p99 = 1000000*p99

        fig, ax = plt.subplots(nrows=1, ncols=1)
        ax.set_title(label="Процентиль p99 = {p99:.2f}мкс\nРазмер буфера {sz} Сообщений/сек {freq}".format(sz = size, freq = freq, p99=p99))
        ax.plot(send, retrieve-send)
        ax.set_xlabel("Время [сек]")
        ax.set_ylabel("Время отклика [мкс]")
        ax.set_yscale('log')
    
        file_name = "img/test_res_" + size + "_" + freq + ".png"
        print(file_name)
        fig.savefig(file_name)
        plt.close(fig)

        procentile.append(p99)
        mes_per_sec.append(freq)

    return size, mes_per_sec, procentile


def get_statistics(buffer_sizes):

    Statistics = []
    for buffer_size in buffer_sizes:
        size, mes_per_sec, procentile = save_and_plot_res(buffer_size)
        
        Statistic = {'size': size, "freq": mes_per_sec, "p99": procentile}

        Statistics.append(Statistic)

    return Statistics




def generate_general_output(buffer_sizes, save_file_name):

    Statistics = get_statistics(buffer_sizes)


    for i in range(0,len(Statistics)):
        Statistic = Statistics[i]
        size = Statistic['size'] 
        mes_per_sec = Statistic['freq']
        procentile = Statistic['p99']

        plt.plot(mes_per_sec, procentile, label ="Размер буфера = {buff_size}".format(buff_size = buffer_sizes[i]))
        plt.scatter(mes_per_sec, procentile)
        plt.title("Зависимость времени отклика\nот количества сообщений в секунду и размера буфера ")
        
        plt.xlabel("Сообщений/сек")
        plt.ylabel("p99 [мкс]")

    plt.legend()
    plt.savefig(save_file_name)
    plt.close()
    

small_buffer_size = ["4KB", "16KB", "32KB", "256KB"]
mid_buffer_size = ["512KB", "1024KB", "2048KB", "4096KB"]
big_buffer_size = ["8MB", "16MB", "32MB"]



# for buff_sz in big_buffer_size:
#     print(gen_table_meserment(buff_sz))


# for buff_sz in big_buffer_size:
#     print(print_p99_table(buff_sz))


generate_general_output(small_buffer_size, "small_buffer_size.png")
generate_general_output(mid_buffer_size, "mid_buffer_size.png")
generate_general_output(big_buffer_size, "big_buffer_size.png")


