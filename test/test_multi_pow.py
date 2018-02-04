import ctypes
import sys
import _thread
import time

NUM_TRYTES = 100

join_list = []
for i in range(NUM_TRYTES):
    join_list.append(_thread.allocate_lock())

def read_trytes():
    f = open('./test/trytes_list_200.txt', 'r')
    row = f.readlines()
    tmp = []
    for r in row:
        tmp.append(r.split('\n')[0])

    return tmp

def call_dcurl(idx, mwm):
    tmp = str(trytes_list[idx]).encode('ascii')
    libdcurl.dcurl_entry(tmp, mwm)
    join_list[idx].release()

if __name__ == "__main__":
    num_cpu = int(sys.argv[1])
    num_gpu = int(sys.argv[2])
    
    trytes_list = read_trytes()

    dcurlPath = "./libdcurl.so"

    libdcurl = ctypes.cdll.LoadLibrary(dcurlPath)

    libdcurl.dcurl_init.argtypes = [ctypes.c_int, ctypes.c_int]

    # (size of cpu queue, size of gpu queue 
    libdcurl.dcurl_init(num_cpu, num_gpu)

    libdcurl.dcurl_entry.argtypes = [ctypes.c_char_p, ctypes.c_int]
    
    for i in range(NUM_TRYTES):
        join_list[i].acquire()
        _thread.start_new_thread(call_dcurl, (i, 14, ))

    for i in range(NUM_TRYTES):
        while join_list[i].locked(): pass

    libdcurl.dcurl_destroy()
