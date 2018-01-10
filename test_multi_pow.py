import ctypes
import sys
import _thread
import time

def read_trytes():
    f = open('./trytes_list_200.txt', 'r')
    row = f.readlines()
    tmp = []
    for r in row:
        tmp.append(r.split('\n')[0])

    return tmp


def call_dcurl(idx, mwm):
    tmp = str(trytes_list[idx]).encode('ascii')
    libdcurl.dcurl_entry(tmp, mwm)
        

if __name__ == "__main__":
    trytes_list = read_trytes()

    dcurlPath = "./libdcurl.so"

    libdcurl = ctypes.cdll.LoadLibrary(dcurlPath)

    libdcurl.dcurl_init()


    libdcurl.dcurl_entry.argtypes = [ctypes.c_char_p, ctypes.c_int]

    for i in range(100):
        _thread.start_new_thread(call_dcurl, (i, 14, ))

    time.sleep(60)
