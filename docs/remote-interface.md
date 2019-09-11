# Remote interface
## Introduction

```
 +-----------------------------------------------+
 |               remote interface                |
 |    +----------------------------------------+ |
 |   +----------------------------------------+| |
 |  +----------------------------------------+|+ |
 |  |     RabbitMQ-provided RPC              |+  |
 |  +----------------------------------------+   |
 |            |                     ^            |
 +------------|---------------------|------------+
 +------------|---------------------|------------+
 |            |   RabbitMQ broker   |            |
 |            |                     |            |
 |            |             +------------------+ |
 |            v            +------------------+| |
 |  +------------------+  +------------------+|| |
 |  | incoming queue   |  | private queue    ||| |
 |  |                  |  |                  ||| |
 |  | - trytes         |  | - PoW result     ||| |
 |  | - mwm            |  |                  ||+ |
 |  |                  |  |                  |+  |
 |  +------------------+  +------------------+   |
 |            |                     ^            |
 +------------|---------------------|------------+
              v                     |
   +---------------------------------------------+
  +---------------------------------------------+|
 +---------------------------------------------+|+
 |                  remote worker              |+
 +---------------------------------------------+
```
To support asynchronous remote procedure call, remote interface in dcurl provides an interface named as `Remote_ImplContext` to implement it.\
dcurl currently uses RabbitMQ C client to implement asynchronous RPC in remote interface.\
Remote interface provides thread management to support an asynchronous RPC per thread.

Here are detailed implementations of the RabbitMQ-provided RPC pattern as follows:
* Asynchronous RPC requests are inserted into the message queue, `incoming_queue`, in RabbitMQ broker
* Asynchronous RPCs with exclusive private queues (callback queues) with TTL = 10s property
* Correlation ID is not used
* An asynchronous RPC uses a connection to RabbitMQ broker
* Remote workers can obtain requests from `incoming_queue` by default exchange of RabbitMQ broker

The remote worker can be executed on the different hardwares and do the PoW.\
For executing it on DE10-Nano board, please read the [docs/board-de10-nano.md](board-de10-nano.md).

## How to test remote interface in localhost
You need to open three terminals

**Terminal 1:**\
Run the RabbitMQ broker\
You can quickly use docker to run the RabbitMQ broker, rabbitmq
```
$ sudo docker run -d -p 5672:5672 rabbitmq
```

---

**Terminal 2:**\
Build and run the remote worker
```
$ cd dcurl
$ make BUILD_REMOTE=1 BUILD_DEBUG=1
$ ./build/remote-worker
```

---

**Terminal 3:**\
Run the tests to send requests to remote worker
```
$ cd dcurl
$ make BUILD_REMOTE=1 BUILD_DEBUG=1 check
```

## Requirements
Remote interface requires RabbitMQ broker
