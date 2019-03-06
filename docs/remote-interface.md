# Remote dcurl

## Introduction

Remote dcurl is inspired by [iotaledger's PoWbox](https://github.com/iotaledger/powbox). Remote dcurl supports asynchronous remote procedure call. We create two queues in RabbitMQ broker that manages queues and use [RabbitMQ C client](https://github.com/alanxz/rabbitmq-c) library to implement dcurl workers that do not compute accelerated PoW until users send new PoW tasks to the work queue (incomming queue) in RabbitMQ broker. In addition, dcurl worker-computed PoW result is sent to the queue (completed queue) in RabbitMQ broker and users can get PoW result from the queue. Users can see test-remotedcurl-new-task.c and test-remotedcurl-get-result.c to understand how to send a new PoW task and get a PoW result to/from work queues. By the way, It can surpport a scenario of a broker and many dcurl workers.

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

## Limitations

* Remote dcurl requires RabbitMQ broker
