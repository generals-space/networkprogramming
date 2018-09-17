## 关于tornado

在最近一次看了tornado官方文档后, 发现ta并没有什么优势可言.

tornado官方文档分为几个部分, 包括异步web框架, 异步http客户端与服务端, 异步socket函数工具集, 事件循环及协程定义工具集.

异步web框架可用sanic代替, 异步http客户端与服务端类似于aiohttp, 而异步socket与事件循环, 可以用asyncio代替. tornado只是这些库的集合而已, 原生库的出现让我觉得没必要使用tornado.

## 关于示例

2018-09-17

tornado的示例07参考了官方示例[Queue example - a concurrent web spider](https://www.tornadoweb.org/en/stable/guide/queues.html), 模仿了示例05, 实现了类似生产者与消费者的过程. 不过这次没有通过单独开线程去维护一个事件循环, 而是用协程 + 队列实现. 

示例05是子线程维护事件循环, 主线程不停获取新任务, 然后创建协程对象丢给事件循环对象, 即, 两个线程间共享的是事件循环对象. 从这一点上看, 示例05其实是不太理想的. 需要改进.