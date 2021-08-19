# Mymuduo
一个基于reactor模式的多线程并发服务器模型的简单实现，学习来源于陈硕的muduo，摆脱了boost库的依赖。 
### 项目表述
该项目主要实现了在一个Reactor的模式下的Channel-EventLoop-Epoller的多线程并发服务器
采用了一个io线程来进行reactor监控，但是实际上具体的eventloop中已经支持多个reactor监控的可能，  
![EOZ00LQEDH)T28F2V9HR_LB](https://user-images.githubusercontent.com/86883267/129748228-758a3acc-4817-4314-b9e9-3c97cb4046ab.png)  
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
### 主要结构与思路简介   
1.声明与接口类:IMuduoUser,IChannelCallback,IAcceptorCallback,IRun,Define.h,Declear.h  
2.时间相关类:Timestamp,Timer,TimerQueue  
3.条件变量锁相关类:MutexLock,MutexLockGuard,Condition,BlockQueue  
4.任务线程类:Task,Thread,ThreadPool  
5.Reactor三剑客类:EventLoop,Channel,Epoll  
6.接受与连接类、缓冲区类:Acceptor,TcpConnection,Buffer
7.终端服务器类:TcpServer,EchoServer  
8.Makefile及Readme   

### 一、声明与接口类  
**IMuduoUser：** 用户注册的回掉函数虚基类，主要是onmessage，onconnect，onwirtecomplete 后续的TcpServer，EchoServer对其有继承  
**IChannelCallback，IAcceptorCallback：** 主要时handlewrite/read的接口虚基类，再channel执行handelevent时会回调该函数，后面Acceptor，Tcpconnection，TimerQueue对有继承   
**IRun：** 任务类的总体描述，包括了读任务和写任务，在线程和线程池类中有判断 Thread，Threadpool  
**Define.h,Declear.h:** Define.h进行了宏定义 Declear.h使得所有的类可以预先声明  

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
Channel类是在eventloop中注册的唯一方式，它我们在epoll中使用的是指针，指针指向的就是Channel，Channel中有用户注册的回调函数，  
例如Accept中的handleread/write或者tcpconnector中的或者timerqueue中的，一个channel中对应了一个fd，可以是通信fd可以是监听fd可以时timerfd  
Eventloop与channel时一对多的关系。Channel中还有enablereading，enablewriting等函数他们分别让可读可写事件注册在事件循环中。  
**3.Epoll类：**
Epoll时内部有epoll相关的函数，其数据成员包括常用的epollfd树根文件描述符，event结构体数组用来接受活跃的事件  
event数据结构中是用指针来指向挂载在epoll树上的channel，创建Epoll类的时候调用了epoll_create，  
主要的函数有两个一个是update暨从channel调用enable传入事件循环，事件循环调用update，epoll调用update调用内和的epollc_ctl  
第二个是poll函数轮询返会内核中活跃的事件，它在Eventloop的loop中调用事件循环开始，poll就不断轮询，到eventloop中进行处理  

三个类别的传入传出过程如下图所示：  
![5%_7)EBB(4 Y$EDVY46$EJC](https://user-images.githubusercontent.com/86883267/129735960-d008f5ac-3b1f-457c-b4a8-c6b9224216de.png)   
Channel进行读事件调用enableread方法，在设定事件文EPOLLIN时同时调用自己的update方法，自己的update方法调用eventloop中的update方法，
事件循环中的update调用了epoller类中的update内部用epoll_ctl来实现这样就是实现了文件的内核自动监听  
![$66N04XH7PGF7$)@TWU5{}A](https://user-images.githubusercontent.com/86883267/129735975-e33edb6b-cfe5-4693-87f5-fec0b54af3df.png)  
当loop开始循环时epoll_wait循环检测并返回活跃的channel这时候，活跃的Channel就自动调用handlevent函数，handlevent函数内部自动调用channel所属的  
handelread和handelwrite函数，例如acceptor中的newconnect，tcpconnect设定的回调函数，timerqueue的回调函数等

### 六、接受与连接类、缓冲区类
接受区连接类主要有Acceptor接受类，Tcpconnection连接类，还有之前所描述的TimerQueue类，他们都是由Channel类进行监控并且在返回活跃的事件时由handelevent  
返回handleread/write进行执行，然后再执行用户注册的回调函数总的来说他们实现了handleread和write接口，针对不同事件的到来做出了不同的响应  
而缓冲区类则是在应用层的层面上对未写完事件进行缓冲  
**1.Accpetor类：**   
接受者类用于接受连接所以首先在内部对ip地址和端口进行绑定，数据的成员一个是用于监听的文件fd与channel还有channel所在的事件循环，还有注册了handelread函数，  
在新的连接到来时被调用会接收它产生一个新的fd，在tcpserve中去创建一个新的tcpconnect对象，并完成注册我们可以在后续的流程图看到整个的回调过程，Accpetor类主要就是进行接收并建立新的连接  
**2.TcpConnection类：**
主要是用来进行通信的类，当由可读事件产生时，就会进行读写操作，由于我们是Lt模式的在读写的过程中可能没有写完，所以我们读写都是针对于缓冲区来进行  
如果读写完成了就会关闭通信fd，在调用handleread和handelwrite的时候也会调用用户的回调函数ImoduoUser。TcpConnection类的生命周期与Accpetor和TimerQueue类一样  
都是channnel与fd注销之后他们也注销  
下面来看这三个类的时序图   
![GQT$`V(2UA$TRQ9HVX77}Y9](https://user-images.githubusercontent.com/86883267/129927340-ce479d2f-0389-4a56-8912-c5c13a77eed9.png)  

**3.Buffer缓冲区类：**   
缓冲区对于我们LT模式是必须的，当前主流的多路复用模型中大多数都是采用的LT模式，与ET模式模式相比各有各的好处，ET模式一次性的把消息发完直到到达饥饿状态，这样很大程度  
上减小了事件注册的次数不会向LT反复注册但是ET模式也有坏处，就是内核至少会询问两边以检查是否真正完成，而LT模式可以不用，明显的减少了调用的次数，但是相应的也应该加入  
输入与输出的缓冲区，主要是写事件（读的过程基本不会出现缓冲以读满但是我们还没有读的情况），在写事件没有写完时就应该加入的缓冲区当中，直到下次可写时再写入内核，这样就  
会大大的减小丢包过程，最大程度上减小读写的bug  

### 七、终端服务器类   
**1.TcpServer类：**  
TcpServer类中就已经极大程度上集成了服务器类和外部的接口，其中数据成员包括了一个Acceptor接收类，  
多个TcpConnection类，再一个map中储存了id与连接的制作，由于实现onconnect是在server中实现的它还继承了user类，来实现其回调函数，再这里面就是建立显得连接  
并完成注册将文件描述符上树等   

**2.EchoServer类：**
EchoServer终端服务器类是我们连接外部的接口，它内部包含有所在的EventLoop有Tcp服务器，还有Threadpool线程池，也继承了user类再最外部，规定通信时和定时器类的回调函数  
分模块图如图所示  
![E}DA(W ODEEL72~ SI1_I{7](https://user-images.githubusercontent.com/86883267/129930945-08181987-d648-45d0-b7da-2123fe1a0084.png)  

### 总类别关系图
其中白色菱形是一个拥有多个的聚合关系，黑色菱形时集合一对一的拥有关系  
![类图 (1)](https://user-images.githubusercontent.com/86883267/129744315-064aac79-e8e9-453d-b230-a3d35a7607ca.png)  

### 压力测试  
由于还没实现http的报文解析和回传，所以暂时只用了webbench进行了连接的压力测试，我们用get方法只获取连接，连接成功后返回  
由500，1000，1500个客户端在10秒内发起连接可以看到Mymuduo能够很好的在短时间内连续的处理并发请求，平均每个客户端，可以在10s内成功进行4-5次连接    
![339{%$1@0IA6OKSYG)M8ISK](https://user-images.githubusercontent.com/86883267/129996759-f5dad0b0-be2e-42fd-8aa1-08aaa37dc94a.png)  


### 其他拓展
由于本项目时在一个reactor中实现的，没有用到多个reactor互相调用的过程，但是我们实现了在同一个线程和在其他线程完成的可能性  
分别时Eventloop中的queueinloop方法和runinloop方法   
runinloop是判断调用的该loop是否是当前线程的loop，如果是当前的线程立即执行，如果不是当前线程先加入到queue队列中去  
相应在eventloop中去处理返回的活跃事件时，我们首先先处理返回回来的handleevent然后还要处理在此线程等待的其他线程的任务，  
如果在轮询处理其他线程所属任务队列中的会进行判断，如果当前线程正在执行之前callback还会阻塞，等待流程图如下  
![OV4J8295$X7` `P7KYG51FP](https://user-images.githubusercontent.com/86883267/129757690-12ebaeec-095a-4dfb-a900-1b32ec35e6fc.png)

### 不足和未来的补充
本项目由许多不足和有待完善之处，  
1.只是做了简单的回射并还没有实现web服务的过程，可以在handlewrite和handleread更改Tcpconnect处的接口实现分别做成解析http报文，和发送http报文的过程就好了
2.在有输入输出缓冲区还有没完全的利用上，由于现在还没有发送实体内容，几乎不可能出现内核发送缓冲区已满需要缓存到发送缓冲区的过程LT模式的优势之处还没有完全展现  
3.定时连接在压力测试时有时会出现文件已满的请况，解决的方案由两种，第一个是在Linux系统下运用ulimit -n 查看用户进程中最大的文件描述符数量，默认是1024个，我们可以更改这个数量  
在系统的配置文件中/etc/security/limits.conf，本人方法比较暴力直接给改成了65535个（这样不好），第二个是在逻辑层面上更改，准备一个空的文件fd在连接数量已满上限的千情况下，  
利用空的fd来接受连接然后不提供服务马上断开实现完美关闭。    
4.本文虽然实现了One-loop-per-thread的思想，但是最终还是使用了一个人reactor，后期可以加上多个reactor，主reactor接受连接，然后将通信的fd联通channel发送给其他reactor
其他subreactor也都配有线程池这样就能实现reactor池+线程池模型，完成最优的多线程并发web服务器模型    
5.实现压力测试！！！！！

虽然没有完全实现上述功能，但是在学习多线程并发服务器设计的过程中，学习了许多设计思想，RAII封装，Threadlocal变量等学习到了很多 感谢！  
如有建议和意见欢迎联系19s053048@stu.hit.edu.cn邮箱交流
