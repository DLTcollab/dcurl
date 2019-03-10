# Remote interface

## Introduction

```
 +---------------------------------------------+
 |                   Client                    |
 |  +------------------+  +------------------+ |
 |  |  send new work   |  |    get result    | |
 |  +------------------+  +------------------+ |
 |           |                     ^           |
 +-----------|---------------------|-----------+
 +-----------|---------------------|-----------+
 |           v   RabbitMQ broker   |           |
 |  +------------------+  +------------------+ |
 |  | incomming queue  |  | completed queue  | |
 |  |                  |  |                  | |
 |  | - trytes         |  | - PoW result     | |
 |  | - mwm            |  |                  | |
 |  |                  |  |                  | |
 |  +------------------+  +------------------+ |
 |           |                     ^           |
 +-----------|---------------------|-----------+
             v                     |
 +---------------------------------------------+
 |                 dcurl worker                |
 +---------------------------------------------+
```

Remote interface is inspired by [IOTA's PoWbox](https://github.com/iotaledger/powbox). Remote interface supports asynchronous remote procedure call. We create two queues (incomming queue, completed queue) in RabbitMQ broker that can manages queues. Dcurl worker-required data is stored in the incomming queue by sending remote call of new work from client. After dcurl worker computes accelerated PoW, it sends remote call to store the PoW result in the completed queue in RabbitMQ broker. Moreover, we use [RabbitMQ C client](https://github.com/alanxz/rabbitmq-c) library to implement dcurl workers that do not compute accelerated PoW until users send data of new PoW tasks to the incomming queue in RabbitMQ broker. On the other hand, Users can see test-remote-new-task.c and test-remote-get-result.c to understand how to send a new PoW task and get a PoW result to/from work queues. By the way, It can surpport a scenario of a broker and many dcurl workers.

## How to test remote dcurl in localhost

You need to open four terminals

Terminal 1: Run the RabbitMQ broker
You can quickly use docker to run the RabbitMQ broker, [rabbitmq](https://hub.docker.com/_/rabbitmq)
```
sudo docker run -d rabbitmq
```

Terminal 2: Create new PoW task
```
$./build/test-remotedcurl-new-task
```

Terminal 3: Run remote dcurl
```
$./build/remotedcurl
```

Terminal 4: Get result of remote dcurl and verify PoW result
```
$./build/test-remotedcurl-get-result
```

## Requirements

* Remote dcurl requires RabbitMQ broker
