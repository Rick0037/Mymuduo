# Mymuduo
一个基于reactor模式的多线程并发服务器模型的简单实现，学习来源于陈硕的muduo，摆脱了boost库的依赖。 
### 项目表述
该项目主要实现了在一个Reactor的模式下的Channel-EventLoop-Epoller的多线程并发服务器  
采用了Epolle的LT模式，作为IO复用的模型，在应用层的层面上加入了输入和输出的缓冲区   
使用了ThreadPool来进行任务的处理，和注册的定时器Timer类，TimerQueue类  
### 使用过程
在文件夹下打开
```
cd Mymuduo
make clean
make
./Mymuduo
```
启动另个一终端输入   
```
nc 127.0.0.1 10037
```
就可以开始通信了   
### 主要结构简介   
1.声明与接口类:IMuduoUser,IChannelCallback,IAcceptorCallback,IRun,Define.h,Declear.h  
2.时间相关类:Timestamp,Timer,TimerQueue  
3.条件变量锁相关类:MutexLock,MutexLockGuard,Condition,BlockQueue  
4.任务线程类:Task,Thread,ThreadPool  
5.Reactor三剑客类:EventLoop,Channel,Epoll  
6.接受与连接类、缓冲区类:Acceptor,TcpConnection,Buffer
7.终端服务器类:TcpServer,EchoServer  
8.Makefile及Readme  
### 一、声明与接口类
### 二、时间相关类
时间相关主要有三个类别Timestamp时间戳类，Timer定时器类，TimerQueue定时器队列   
**1.Timestamp时间戳类:** 
采取了六十四位的int数据来储存时间，实现了获取当前时间，时间比较判断时间是否有效的方法。   
**2.Timer是定时器类:**  
定时器类以时间戳为变量，可以用当前时间或者任意时间来进行定时，并设定是否需要重复开启  
**3.TimerQueue定时器队列类:**  
定时器队列类是有一系列的定时器，当一些定时器过期时我们可以选择是否继续设置或更新，  
定时器队列使用了一个timerfd注册一个channel进行监控，在eventloop轮询使若有定时器到期就会返回  
eventloop中会调用handlevent来调用可读回调函数handleread在定时器队列整个channel中就会调用  
getExpired获取过期的定时器然后根据是否重复来reset重置时间定时器，并回调直接定时任务
![XHW~TZKW%H%P3C(H2{S4$32](https://user-images.githubusercontent.com/86883267/129739055-5b479931-b111-4ff1-99c8-d63e57a9865b.png)  

### 三、条件变量锁相关类
条件变量和锁主要有四个类别MutexLock互斥量,MutexLockGuard互斥量实例RAII,Condition条件变量类,BlockQueue无界的缓冲区  
**1.MutexLock互斥量：**  
由于我们是线程来执行任务的，我们的互斥量内部采用的是pthread_mutex类别的  
构造函数调用pthread_mutex_init,析构函数调用pthread_mutex__destory  
在其内部lock与unlock 分别调用pthread_lock/unlock  
**2.MutexLockGuard互斥量实例：**  
采取的是RAII封装技法，在MutexLockGuard类中包含了一个Mutex对象  
在构造时进行创建加锁，在析构时自动解锁推出  
**3.Condition条件变量类：**   
条件变量类内部也维护了一个pthread_cond_it也自动的调用内部的互斥量进行wakeup和signal操作  
**4.BlockQueue无界的缓冲区：**
无界缓存区类则是完全的模拟了生产者和消费者的模型，put和take方法打配内部的条件变量，  
当缓冲区为空的时候不能进行take操作wait阻塞线程，当put放置任务时signal唤醒线程  
![$OIICPEM3_ UE2QY%GG4{S8](https://user-images.githubusercontent.com/86883267/129739609-1143b9c9-df8f-4b4f-bef2-d73973afb892.png)  

### 四、任务线程类
任务和线程类主要有以下的三种类别Task任务类,Thread线程类,ThreadPool线程池类  
**1.Task任务类：**  
重载了两个构造函数，模拟了读写过程锁执行的两种任务  
**2.Thread线程类：**  
完成了pthread_create的创建线程并执行任务的功能，还提供了获取tid的方法  
**3.ThreadPool线程池类：**  
内部有一个队列任务池和一个vector线程池，threadpool进行start方法后线程就会到任务池区领取任务并执行  
![$FBL6BLTG)%W_68P4$TMLJ5](https://user-images.githubusercontent.com/86883267/129739703-da469651-330a-4a55-aa73-2394133ab843.png)  

### 五、Reactor三剑客类
Reactor模式在本项目的实现主要由三个类来实现   
**1.eventloop事件循环类：**
事件循环是一个单一的loop过程每个线程类可能只有一个事件循环，事件循环是不断来轮询事件  
内核检测活跃的事件并返回，如果是epollin就返回channel调用可读事件的回调函数，如果监测出了epollout说明有可写事件发生  
例如在LT模式下未发送完写入了应用层的缓冲区，缓冲区可写就会发生写事件，此时eventloop就会返回channel并调用可写函数  
**2.Channel通道类：**

**3.Epoll类：**
Epoll时内部有epoll相关的函数，其数据成员包括常用的epollfd树根文件描述符，event结构体数组用来接受活跃的事件  
event数据结构中是用指针来指向挂载在epoll树上的channel，创建Epoll类的时候调用了epoll_create，  
主要的函数有两个一个是update暨从channel调用enable传入事件循环，事件循环调用update，epoll调用update调用内和的epollc_ctl  
第二个是poll函数轮询返会内核中活跃的事件，它在Eventloop的loop中调用事件循环开始，poll就不断轮询，到eventloop中进行处理  

三个类别的传入传出过程如下图所示：  
![5%_7)EBB(4 Y$EDVY46$EJC](https://user-images.githubusercontent.com/86883267/129735960-d008f5ac-3b1f-457c-b4a8-c6b9224216de.png)  
![$66N04XH7PGF7$)@TWU5{}A](https://user-images.githubusercontent.com/86883267/129735975-e33edb6b-cfe5-4693-87f5-fec0b54af3df.png)

### 六、接受与连接类、缓冲区类
### 七、终端服务器类
