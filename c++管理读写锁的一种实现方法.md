## C++管理读写锁的一种实现方法
读写锁是一种常用的线程同步机制，在多线程环境下访问同一内存区域时往往会用到，本篇不是介绍读写锁的原理，而是在假设读者都知道读写锁的基本使用方式的前提下，介绍一种管理读写锁的方法  
###读写锁的基本使用
===
为了读起来好理解，还是先介绍一下基本概念和使用  

**基本概念**  

读写锁有三种状态：读模式加锁，写模式加锁，不加锁

读写锁的使用规则  

1. 在当前没有写锁的情况下，读者可以立马获取读锁  
2. 在当前没有读锁和写锁的情况下，写者可以立马获取写锁

也就是说，可以多个读者同时获取读锁，而写者只能有一个，且在写的时候不能读

**基本使用**  

初始化和销毁

	int pthread_rwlock_init(pthread_rwlock_t *restrict rwlock, const pthread_rwlockattr_t *restrict attr);
	int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
读和写

	int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
	int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
另外还有非阻塞模式的读写

	int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
	int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
解锁

	int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
**示例**

test.cpp

	#include <stdio.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <pthread.h>

	#define  ARRAY_SIZE 10
	int g_array[ARRAY_SIZE];
	pthread_rwlock_t g_mutex;

	void* thread_func(void* arg)
	{
		while (true)
		{
        	int index = random() % ARRAY_SIZE;
        	if (0 == random() % 2) 
        	{
           		// read
           		pthread_rwlock_rdlock(&g_mutex);
           		printf("read array[%d]:%d\n", index, g_array[index]);
        	}
        	else
        	{
        		// write
        		pthread_rwlock_wrlock(&g_mutex);
        		int value = random() % 100;
        		g_array[index] = value;
        		printf("write array[%d]:%d\n", index, value);
        	}
        	pthread_rwlock_unlock(&g_mutex);
        	sleep(2);
    	}
    }	
    int main(int argc, char* argv[])
    {
    	pthread_rwlock_init(&g_mutex, NULL);
    	srand((unsigned)time(NULL));
    	int pthread_num = 10;
    	pthread_t threads[pthread_num];
    	for (int i = 0; i < pthread_num; ++i)
    	{
    		pthread_create(&threads[i], NULL, thread_func, NULL);
    	}
    	for (int i = 0; i < pthread_num; ++i)
    	{
        	pthread_join(threads[i], NULL);
    	}
    	pthread_rwlock_destroy(&g_mutex);
    	return 0;
    }
这段代码可以直接编译运行

	g++ test.cpp -o test
	./test
	
###读写锁的管理
===
通过上面的代码我们可以了解读写锁的基本使用方法，在需要读的时候调用读锁命令，需要写的时候调用写锁命令，读写完后调用解锁命令，这样使用虽然简单易懂，但是有时候会让代码很繁琐，比如当你调用了读锁命令后，程序可能会有多个出口，如果不使用goto语句的话（goto语句在某些编程规范里是明令禁止的，苹果曾经因为goto语句导致SSL连接验证的bug，有一篇文章分析得很好，可以参考一下 [由苹果的低级Bug想到的编程思考](http://mobile.51cto.com/hot-431352.htm)），那你需要在每个出口都调用一次解锁操作，这样就失去了程序的优雅性，我们用下面的伪代码片段来描述这种情况

	int func()
	{
		pthread_rwlock_rdlock(&g_mutex);
		if (condition1)
		{
			// do something
			pthread_rwlock_unlock(&g_mutex);
			return -1
		}
		else if(condition2)
		{
			// do something
			pthread_rwlock_unlock(&g_mutex);
			return -2;
		}
		else
		{
			// do something
		}
		pthread_rwlock_unlock(&g_mutex);
		return 0;
	}
这个程序有多个出口，在每个出口我们都要手动调用一次解锁，很显然这不是我们期望的样子，那理想的情况应该是怎样的呢，它应该是只需显式的调用一次加锁操作，在每个出口会自动调用解锁，于是我们很容易想到用类来管理，在程序入口定义一个类对象，在构造函数里调用加锁操作，当程序return后，类对象会自动析构，我们在析构函数里实现解锁，这样就不用每次手动去调用解锁操作了。管理读写锁的类的实现如下，我们把代码放在头文件rwlock.h下
	
	#ifndef _RWLOCK_H_
	#define _RWLOCK_H_
	
	#include <iostream>
	#include <pthread.h>


	enum ELockType
	{
    	READ_LOCKER,
    	WRITE_LOCKER, 
	};

	class RWLockManager;

	class RWLock
	{
	public:
    	typedef RWLockManager Lock;
    	RWLock()
    	{
        	pthread_rwlockattr_t attr;
        	pthread_rwlockattr_init(&attr);
        	pthread_rwlock_init(&m_rwlock, &attr);
    	}
    	virtual ~RWLock()
    	{
        	pthread_rwlock_destroy(&m_rwlock);
    	}
    	int rdlock()
    	{
    		return pthread_rwlock_rdlock(&m_rwlock);
    	}
    	int wrlock()
    	{
        	return pthread_rwlock_wrlock(&m_rwlock);
    	}
    	int unlock()
    	{
     	   return pthread_rwlock_unlock(&m_rwlock);
     	}

	private:
    	pthread_rwlock_t m_rwlock;
	};

	class RWLockManager
	{
		public:
    	RWLockManager(RWLock& locker, const ELockType lock_type) : m_locker(locker)
    	{
        	if (lock_type == READ_LOCKER)
        	{
            	int ret = m_locker.rdlock();
            	if (ret != 0)
            	{
                	std::cout << "lock failed, ret: " << ret;
            	}
        	}
        	else
        	{
            	int ret = m_locker.wrlock();
            	if (ret != 0)
            	{
            		std::cout << "lock failed, ret: " << ret;
            	}
        	}
    	}
    	~RWLockManager()
    	{
    		int ret = m_locker.unlock();
    		if (ret != 0)
    		{
    			std::cout << "unlock failed, ret: " << ret;
    		}
    	}
    	private:
    		RWLock& m_locker;
    };
    #endif
注意类RWLockManager的成员变量m_lock必须是一个RWLock类型的引用

这样在使用起来的时候就很简单明了，还是上面那份伪代码，使用读写锁管理类之后如下
	
	int func()
	{
		RWLock::Lock lock(g_mutex, READ_LOCKER);
		if (condition1)
		{
			// do something
			return -1
		}
		else if(condition2)
		{
			// do something
			return -2;
		}
		else
		{
			// do something
		}
		return 0;
	}
###总结
===
以上就是管理读写锁的一种方式，只要稍微花点心思就可以让代码变得简洁清晰，程序的根本目的是消除重复劳动，如果我们在写代码的时候要重复写了很多遍某些语句，那么我们就应该考虑是不是设计一个方法消除这种重复。