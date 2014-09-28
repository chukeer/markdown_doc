#让你的程序更优雅的sleep

sleep的作用无需多说，几乎每种语言都提供了类似的函数，调用起来也很简单。sleep的作用无非是让程序等待若干时间，而为了达到这样的目的，其实有很多种方式，最简单的往往也是最粗暴的，我们就以下面这段代码来举例说明（**注：本文提及的程序编译运行环境为Linux**）

	/* filename: test.cpp */
	#include <stdio.h>
	#include <unistd.h>
	#include <pthread.h>
	#include <signal.h>

	class TestServer
	{
	public:
		TestServer() : run_(true) {};
    	~TestServer(){};

    	void Start()
    	{
    		pthread_create(&thread_, NULL, ThreadProc, (void*)this);
    	}
    	void Stop()
    	{
    		run_ = false;
    	}
    	void Wait()
    	{
    		pthread_join(thread_, NULL);
    	}
    	void Proc()
    	{
    		int count = 0;
        	while (run_)
        	{
        		printf("sleep count:%d\n", ++count);
        		sleep(5);
        	}
    	}

	private:
		bool run_;
    	pthread_t thread_;

    	static void* ThreadProc(void* arg)
    	{
    		TestServer* me = static_cast<TestServer*>(arg);
        	me->Proc();
        	return NULL;
    	}
	};

	TestServer g_server;

	void StopService()
	{
		g_server.Stop();
	}

	void StartService()
	{
		g_server.Start();
		g_server.Wait();
	}

	void SignalHandler(int sig)
	{
		switch(sig)
		{
			case SIGINT:
				StopService();
        	default:
        		break;
    	}
    }

	int main(int argc, char* argv[])
	{
		signal(SIGINT, SignalHandler);
    	StartService();
    	return 0;
	}

这段代码描述了一个简单的服务程序，为了简化我们省略了服务的处理逻辑，也就是Proc函数的内容，这里我们只是周期性的打印某条语句，为了达到周期性的目的，我们用sleep来实现，每隔5秒钟打印一次。在main函数中我们对SIGINT信号进行了捕捉，当程序在终端启动之后，如果你输入ctr+c，这会向程序发送中断信号，一般来说程序会退出，而这里我们捕捉到了这个信号，会按我们自己的逻辑来处理，也就是调用server的Stop函数。执行编译命令

	g++ test.cpp -o test -lpthread
	
然后在终端输入`./test`运行程序，这时程序每隔5秒会在屏幕上打印一条语句，按下ctl+c，你会发现程序并没有立即退出，而是等待了一会儿才退出，究其原因，当按下ctl+c发出中断信号时，程序捕捉到并执行自己的逻辑，也就是调用了server的Stop函数，运行标记位run_被置为false，Proc函数检测到run\_为false则退出循环，程序结束，但有可能（应该说大多数情况都是如此）此时Proc正好执行到sleep那一步，而sleep是将程序挂起，由于我们捕捉到了中断信号，因此它不会退出，而是继续挂起直到时间满足为止。这个sleep显然显得不够优雅，下面介绍两种能快速退出的方式。

###自定义sleep
在我们调用系统提供的sleep时我们是无法在函数内部做其它事情的，基于此我们就萌生出一种想法，如果在sleep中能够检测到退出变量，那岂不是就能快速退出了，没错，事情就是这样子的，通过自定义sleep，我们将时间片分割成更小的片段，每隔一个片段检测一次，这样就能将程序的退出延迟时间缩小为这个更小的片段，自定义的sleep如下

	void sleep(int seconds, const bool* run)
	{
		int count = seconds * 10;
		while (*run && count > 0)
		{
			--count;
			usleep(100000);
		}
	}
	
需要注意的是，这个sleep的第二个参数必须是指针类型的，因为我们需要检测到它的实时值，而不只是使用它传入进来的值，相应的函数调用也得稍作修改，完整的代码如下

	/* filename: test2.cpp */
	#include <stdio.h>
	#include <unistd.h>
	#include <pthread.h>
	#include <signal.h>

	class TestServer
	{
	public:
    	TestServer() : run_(true) {};
    	~TestServer(){};

    	void Start()
    	{
       	 pthread_create(&thread_, NULL, ThreadProc, (void*)this);
    	}

    	void Stop()
    	{
     	   run_ = false;
    	}

    	void Wait()
    	{
    	    pthread_join(thread_, NULL);
   	 	}

    	void Proc()
   	 	{
	      	int count = 0;
	     	while (run_)
	     	{
		      	printf("sleep count:%d\n", ++count);
		   		sleep(5, &run_);
	   	   	}
    	}
    
    private:
    	bool run_;
    	pthread_t thread_;

    	void sleep(int seconds, const bool* run)
    	{
        	int count = seconds * 10;
        	while (*run && count > 0)
        	{
            	--count;
            	usleep(100000);

        	}
    	}

    	static void* ThreadProc(void* arg)
    	{
        	TestServer* me = static_cast<TestServer*>(arg);
        	me->Proc();
        	return NULL;
    	}
	};

	TestServer g_server;

	void StopService()
	{
 	   g_server.Stop();
	}

	void StartService()
	{
	    g_server.Start();
 	   g_server.Wait();
	}

	void SignalHandler(int sig)
	{
	    switch(sig)
	    {
	    	case SIGINT:
    	    	StopService();
    	    default:
    	    	break;
   	 	}
	}

	int main(int argc, char* argv[])
	{
    	signal(SIGINT, SignalHandler);
    	StartService();
    	return 0;
	}
	
编译`g++ test2.cpp -o test`，运行`./test`，当程序启动之后按`ctl+c`，看程序是不是很快就退出了。

其实这种退出并不是立马退出，而是将sleep的等待时间分成了更小的时间片，上例是0.1秒，也就是说在按下ctr+c之后，程序其实还会延时0到0.1秒才会退出，只不过这个时间很短，看上去就像立马退出一样。

###用条件变量实现sleep

大致的思想就是，在循环时等待一个条件变量，并设置超时时间，如果在这个时间之内有其它线程触发了条件变量，等待会立即退出，否则会一直等到设置的时间，这样就可以通过对条件变量的控制来实现sleep，并且可以在需要的时候立马退出。

条件变量往往会和互斥锁搭配使用，互斥锁的逻辑很简单，如果一个线程获取了互斥锁，其它线程就无法获取，也就是说如果两个线程同时执行到了`pthread_mutex_lock`语句，只有一个线程会执行完成，而另一个线程会阻塞，直到有线程调用`pthread_mutex_unlock`才会继续往下执行。所以我们往往在多线程访问同一内存区域时会用到互斥锁，以防止多个线程同时修改某一块内存区域。本例用到的函数有如下几个，互斥锁相关函数有

	int pthread_mutex_init(pthread_mutex_t *restrict mutex,const pthread_mutexattr_t *restrict attr);
	int pthread_mutex_lock(pthread_mutex_t *mutex);
	int pthread_mutex_unlock(pthread_mutex_t *mutex);
	int pthread_mutex_destroy(pthread_mutex_t *mutex);
	
以上函数功能分别是初始化、加锁、解锁、销毁。条件变量相关函数有

	int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr);
	int pthread_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict abstime);
	int pthread_cond_signal(pthread_cond_t *cond);
	int pthread_cond_destroy(pthread_cond_t *cond);
	
以上函数功能分别是初始化、超时等待条件变量、触发条件变量、销毁。这里需要解释一下pthread_cond_timedwait和pthread_cond_signal函数

**pthread_cond_timedwait**  
这个函数调用之后会阻塞，也就是类似sleep的作用，但是它会在两种情况下被唤醒：1、条件变量cond被触发时；2、系统时间到达abstime时，注意这里是绝对时间，不是相对时间。它比sleep的高明之处就在第一点。另外它还有一个参数是mutex，当执行这个函数时，它的效果等同于在函数入口处先对mutex加锁，在出口处再对mutex解锁，当有多线程调用这个函数时，可以按这种方式去理解

**pthread_cond_signal**  
它只有一个参数cond，作用很简单，就是触发等待cond的线程，注意，它一次只会触发一个，如果要触发所有等待cond的县城，需要用到pthread_cond_broadcast函数，参数和用法都是一样的

有了以上背景知识，就可以更加优雅的实现sleep，主要关注Proc函数和Stop函数，完整的代码如下

	/* filename: test3.cpp */
	#include <stdio.h>
	#include <unistd.h>
	#include <pthread.h>
	#include <signal.h>
	#include <sys/time.h>

	class TestServer
	{
	public:
		TestServer() : run_(true) 
    	{
    		pthread_mutex_init(&mutex_, NULL);
    		pthread_cond_init(&cond_, NULL);
    	};
    	~TestServer()
    	{
    		pthread_mutex_destroy(&mutex_);
    		pthread_cond_destroy(&cond_);
    	};

    	void Start()
    	{
    		pthread_create(&thread_, NULL, ThreadProc, (void*)this);
    	}

    	void Stop()
    	{
	      	run_ = false;
	      	pthread_mutex_lock(&mutex_);
	      	pthread_cond_signal(&cond_);
	      	pthread_mutex_unlock(&mutex_);
       }

    	void Wait()
    	{
	      	pthread_join(thread_, NULL);
    	}

    	void Proc()
    	{
	    	pthread_mutex_lock(&mutex_);
	    	struct timeval now;
	    	int count = 0;
	    	while (run_)
        	{
	         	printf("sleep count:%d\n", ++count);
	         	gettimeofday(&now, NULL);
	         	struct timespec outtime;
	         	outtime.tv_sec = now.tv_sec + 5;
	         	outtime.tv_nsec = now.tv_usec * 1000;
	         	pthread_cond_timedwait(&cond_, &mutex_, &outtime);
        	}
        	pthread_mutex_unlock(&mutex_);
    	}

	private:
		bool run_;
    	pthread_t thread_;
    	pthread_mutex_t mutex_;
    	pthread_cond_t cond_;

    	static void* ThreadProc(void* arg)
    	{
	      	TestServer* me = static_cast<TestServer*>(arg);
	      	me->Proc();
	      	return NULL;
    	}
    };

	TestServer g_server;

	void StopService()
	{
		g_server.Stop();
	}

	void StartService()
	{
    	g_server.Start();
    	g_server.Wait();
	}

	void SignalHandler(int sig)
	{
    	switch(sig)
    	{
        	case SIGINT:
            	StopService();
        	default:
            	break;
    	}
	}

	int main(int argc, char* argv[])
	{
    	signal(SIGINT, SignalHandler);
    	StartService();
    	return 0;
	}
	
和test2.cpp一样，编译之后运行，程序每隔5秒在屏幕打印一行输出，输入ctr+c，程序会立马退出
