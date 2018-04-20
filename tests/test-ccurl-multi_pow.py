# -*- coding: utf-8 -*-

import ctypes
import sys
import argparse
import _thread
import iota
from iota import TryteString
from iota.crypto import Curl

TRYTES_LIST_PATH = "./tests/trytes.txt"
DCURL_PATH = "./build/libdcurl.so"
NUM_TRYTES = 10
RESULT_TX = []

join_list = []
for i in range(NUM_TRYTES):
    join_list.append(_thread.allocate_lock())

# return list of trytes #
def read_trytes(FILE_PATH):
    f = open(FILE_PATH, 'r')
    row = f.readlines()
    tmp = []
    for r in row:
        tmp.append(r.split('\n')[0])
    return tmp

def hash(trytes):
    curl = Curl()
    curl.absorb(trytes.as_trits())
    trits_out = []
    curl.squeeze(trits_out)
    return TryteString.from_trits(trits_out)

def validate(trytes, mwm):
    trits = trytes.as_trits()
    for i in range(len(trits) - 1, len(trits) - mwm - 1, -1):
        if trits[i] != 0:
            return False
    return True

def call_dcurl(idx, mwm, lib, trytes_list):
    tmp = str(trytes_list[idx]).encode('ascii')
    ret = lib.ccurl_pow(tmp, mwm)
    trytes = TryteString(ret[:2673])

    hash_trytes = hash(trytes)
    RESULT_TX.append(hash_trytes)

    join_list[idx].release()

def testing():
    trytes_list = read_trytes(TRYTES_LIST_PATH)

    # Settings of shared library
    libdcurl = ctypes.cdll.LoadLibrary(DCURL_PATH)
    libdcurl.ccurl_pow.argtypes = [ctypes.c_char_p, ctypes.c_int]
    libdcurl.ccurl_pow.restype = ctypes.c_char_p

    #libdcurl.dcurl_init(num_cpu, num_gpu)

    for i in range(NUM_TRYTES):
        join_list[i].acquire()
        _thread.start_new_thread(call_dcurl, (i, 14, libdcurl, trytes_list, ))

    # threadpool.join()
    for i in range(NUM_TRYTES):
        while join_list[i].locked(): pass 

    libdcurl.ccurl_pow_finalize();

    for tx in RESULT_TX:
        if validate(tx, 14) is not True:
            sys.exit(1)

if __name__ == "__main__":
    testing()
    sys.exit(0)
