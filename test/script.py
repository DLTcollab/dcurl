import os
import time

thread = [8, 16, 32]
task = [8, 9, 10, 11, 12]

'''
fout = open("cpu.txt", "a")

for i in thread:
    for j in task:
        print("thread: " + str(i) + " task: " + str(j))
        os.system("export DCURL_NUM_CPU=" + str(i))
        begin = time.time()
        os.system("python3 test_multi_pow.py " + str(j) + " 0")
        end = time.time()
        fout.write(str(end - begin) + "\n")
fout.close()
'''

fout = open("gpu.txt", "a")

for i in task:
    print("task: " + str(i))
    begin = time.time()
    os.system("python3 test_multi_pow.py 0 " + str(i))
    end = time.time()
    fout.write(str(end - begin) + "\n")
fout.close()
