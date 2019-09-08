# Threading model
In `dcurl`, it calls the APIs provided by the `libtuv` submodule to use the threads and thread pool.

## Thread pool size
The thread pool size can be set with the function `uv_set_threadpool_size()`.\
Default pool size: 4\
Maximum pool size: 128

## Thread pool mechanism
The thread pool and the work request queue are global.\
Work request can be treated as the function you would like to request the thread to execute.

In the thread pool initialization phase, the threads are created and execute the infinite loop function `worker()`.\
The infinite loop makes the thread blocked with the function `uv_cond_wait()`, and it can be unblocked with the function `uv_cond_signal()`.

The function `uv_queue_work()` push the work request into the queue and unblock one of the blocked threads.
Then the unblocked thread pops out the work request from the queue and execute it.\
If all the threads are busy, the first thread finishing its job would check the non-empty queue, pop out the work request and execute it.

In `dcurl`, it calls the function `uv_queue_work()` and passes the arguments with the PoW calculation function pointer and necessary data.\
If the thread pool is large enough, the specified number of threads (which is the argument of `dcurl_entry()`) can be unblocked and fully utilized.\
Otherwise, the `dcurl` would not be able to use as many threads as it specified at the same time.

## Thread affinity
The current thread pool mechanism does not take thread affinity into consideration.
