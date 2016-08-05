# C++ & Python 多线程笔记
## C++
### Posix多线程
	
	
	#include <stdio.h>
	#include <stdlib.h>
	#include <pthread.h>
	
	#include <vector>
	
	using std::vector;
	
	void* Proc(void* arg)
	{
	    pthread_t pthread = *(pthread_t *)arg;
	    printf("this is thread %ld\n", pthread);
	}
	
	int main(int argc, char** argv)
	{
	    if (argc != 2)
	    {
	        printf("usage:%s thread_num", argv[0]);
	        return 1;
	    }
	    int thread_num = atoi(argv[1]);
	    vector<pthread_t> threads(thread_num);
	    for (int i = 0; i < thread_num; ++i)
	    {
	        pthread_t pthread;
	        pthread_create(&pthread, NULL, &Proc, &threads[i]);
	        threads[i] = pthread;
	    }
	    for (vector<pthread_t>::iterator it = threads.begin(); it != threads.end(); ++it)
	    {
	        pthread_join(*it, NULL);
	    }
	    return 0;
	}
	

	
### boost多线程

1. 全局函数作为线程函数


		#include <stdio.h>
		#include <stdlib.h>
		#include <pthread.h>
		
		#include <vector>
		#include <string>
		
		#include <boost/thread.hpp>
		
		using std::vector;
		using std::string;
		
		void Proc(int num, const string& str)
		{
		    printf("this is thread %d, say %s\n", num, str.c_str());
		}
		
		int main(int argc, char** argv)
		{
		    if (argc != 2)
		    {
		        printf("usage:%s thread_num", argv[0]);
		        return 1;
		    }
		    int thread_num = atoi(argv[1]);
		    vector<boost::thread*> threads;
		    for (int i = 0; i < thread_num; ++i)
		    {
		        boost::thread* thread = new boost::thread(&Proc, i, "Hello World!");
		        threads.push_back(thread);    
		    }
		    for (vector<boost::thread*>::iterator it = threads.begin(); it != threads.end(); ++it)
		    {
		        (*it)->join();
		    }
		    return 0;
		}



2. 类成员函数作为线程函数


		#include <stdio.h>
		#include <stdlib.h>
		#include <pthread.h>
		
		#include <vector>
		#include <string>
		
		#include <boost/thread.hpp>
		#include <boost/bind.hpp>
		
		using std::vector;
		using std::string;
		
		class Test
		{
		public:
		    void Proc(int num, const string& str)
		    {
		        printf("this is thread %d, say %s\n", num, str.c_str());
		    }
		};
		
		int main(int argc, char** argv)
		{
		    if (argc != 2)
		    {
		        printf("usage:%s thread_num", argv[0]);
		        return 1;
		    }
		    int thread_num = atoi(argv[1]);
		    vector<boost::thread*> threads;
		    for (int i = 0; i < thread_num; ++i)
		    {
		        Test* test = new Test();
		        boost::thread* thread = new boost::thread(boost::bind(&Test::Proc, test, i, "Hello World!"));
		        threads.push_back(thread);    
		    }
		    for (vector<boost::thread*>::iterator it = threads.begin(); it != threads.end(); ++it)
		    {
		        (*it)->join();
		    }
		    return 0;
		}


## Python

### 使用threading模块


	import sys
	import threading
	
	def proc(num, str):
	    print 'this is thread %d, say %s' % (num, str)
	
	def start_threads(thread_num):
	    threads = []
	    for i in range(thread_num):
	        thread = threading.Thread(target=proc, args=(i, "Hello World!"))
	        threads.append(thread)
	        thread.start()
	
	    for thread in threads:
	        thread.join()
	
	if __name__ == '__main__':
	    if len(sys.argv) != 2:
	        print "usage:%s thread_num" % sys.argv[0]
	        sys.exit(1)
	    start_threads(int(sys.argv[1]))

### 使用multiprocessing模块

	import sys
	from multiprocessing.dummy import Pool as ThreadPool
	
	def proc(arg):
	    num, str = arg
	    print 'this is thread %d, say %s' % (num, str)
	
	def start_threads(thread_num):
	    pool = ThreadPool(thread_num)
	    args = [(i, "Hello World!") for i in range(thread_num)]
	    pool.map(proc, args)
	    pool.close()
	    pool.join()
	
	if __name__ == '__main__':
	    if len(sys.argv) != 2:
	        print "usage:%s thread_num" % sys.argv[0]
	        sys.exit(1)
	    start_threads(int(sys.argv[1]))