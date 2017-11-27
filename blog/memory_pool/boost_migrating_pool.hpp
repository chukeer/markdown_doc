#ifndef BOOST_MIGRATING_POOL_HPP
#define BOOST_MIGRATING_POOL_HPP

#include <assert.h>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/pool/poolfwd.hpp>
#include <boost/pool/pool.hpp>
#include <boost/pool/detail/guard.hpp>
#include <boost/type_traits/aligned_storage.hpp>

template <typename Tag,                            // 内存池对象唯一标识，通过不同的Tag可以实例化不同对象，同singleton_pool
   unsigned RequestedSize,                         // 每次请求的内存大小，同singleton_pool
   unsigned MigratingSize = 1024*1024*1024,        // 用户申请的内存峰值超过该值触发内存池迁移条件1
   unsigned MigratingUsedSize = 200 * 1024 * 1024, // 用户当前使用内存小于该值触发内存迁移条件2
   unsigned MigratingDelaySec = 60,                // 同时满足条件1和条件2，并持续该时间（单位秒），才真正触发内存迁移
   unsigned NextSize = 32,                         // 初次申请block时包含的chunk数量，后续每次新申请数量会乘2，同singleton_pool
   unsigned MaxSize = 0>                           // 申请新block时包含的最大chunk数量，为0则代表无上限，同singleton_pool
class migrating_pool
{
public:
    typedef Tag tag;

private:
    migrating_pool();

    // 继承自pool对象，覆盖malloc和free方法，以便做一些统计
    struct pool_type: public boost::pool<boost::default_user_allocator_new_delete>
    {
        pool_type() : boost::pool<boost::default_user_allocator_new_delete>(RequestedSize, NextSize, MaxSize) 
        {
            max_chunk_num = 0;
            cur_chunk_num = 0;
            migrating_max_chunk_num = MigratingSize / RequestedSize;
            migrating_used_chunk_num = MigratingUsedSize / RequestedSize;
            migration_prepared_time = 0;
        }

        void * malloc()
        {
            if (++cur_chunk_num > max_chunk_num)
            {
                max_chunk_num = cur_chunk_num;
            }

            update_migration_status();

            return boost::pool<boost::default_user_allocator_new_delete>::malloc();
        }

        void free(void * const ptr)
        {
            boost::pool<boost::default_user_allocator_new_delete>::free(ptr);
            --cur_chunk_num;
            update_migration_status();
        }

        inline void update_migration_status()
        {
            if (need_migrate())
            {
                if (migration_prepared_time <= 0)
                {
                    migration_prepared_time = time(NULL);
                }
            }
            else
            {
                migration_prepared_time = 0;
            }
        }

        inline void reset()
        {
            cur_chunk_num = 0;
            max_chunk_num = 0;
            migration_prepared_time = 0;
        }

        inline bool empty()
        {
            return (cur_chunk_num == 0);
        }

        inline bool need_migrate()
        {
            return max_chunk_num >= migrating_max_chunk_num && cur_chunk_num <= migrating_used_chunk_num;
        }

        inline void set_migrating_size(int size)
        {
            migrating_max_chunk_num = size / RequestedSize;
            update_migration_status();
        }

        inline void set_migrating_used_size(int size)
        {
            migrating_used_chunk_num = size / RequestedSize;
            update_migration_status();
        }

        int max_chunk_num;               // 分配出去的chunk数量峰值
        int cur_chunk_num;               // 当前分配出去的chunk数量
        int migrating_max_chunk_num;     // max_chunk_num大于该值触发迁移条件1
        int migrating_used_chunk_num;    // cur_chunk_num大于该值触发迁移条件2，条件1和条件2同时满足时才符合迁移条件
        int64_t migration_prepared_time; // 满足迁移条件的时间戳
    };

public:
    static void * malloc BOOST_PREVENT_MACRO_SUBSTITUTION()
    {
        boost::mutex::scoped_lock lock(mutex);
        pool_type & p = get_malloc_free_pool();
        return p.malloc();
    }

    static void free BOOST_PREVENT_MACRO_SUBSTITUTION(void * const ptr)
    { 
        boost::mutex::scoped_lock lock(mutex);

        if (is_migrating)
        {
            pool_type & p = get_free_only_pool();
            if (!p.empty() && p.is_from(ptr))
            {
                p.free(ptr);
                if (p.empty())
                {
                    boost::thread(boost::bind(&clean_pool_proc));
                }
            }
            else
            {
                pool_type & p2 = get_malloc_free_pool();
                p2.free(ptr);
            }
        }
        else
        {
            pool_type & p = get_malloc_free_pool();
            p.free(ptr);

            if (p.migration_prepared_time != 0
                    && time(NULL) - p.migration_prepared_time > migration_delay_sec)
            {
                std::cout << "set is_migrating true" << std::endl;
                int tmp = malloc_free_index;
                malloc_free_index = free_only_index;
                free_only_index = tmp;
                is_migrating = true;

                if (p.empty())
                {
                    boost::thread(boost::bind(&clean_pool_proc));
                }
            }
        }
    }

    static void set_migrating_size(int size)
    {
        boost::mutex::scoped_lock lock(mutex);
        get_malloc_free_pool().set_migrating_size(size);
        get_free_only_pool().set_migrating_size(size);
    }

    static void set_migrating_used_size(int size)
    {
        boost::mutex::scoped_lock lock(mutex);
        get_malloc_free_pool().set_migrating_used_size(size);
        get_free_only_pool().set_migrating_used_size(size);
    }

    static void set_migrating_delay_sec(int sec)
    {
        boost::mutex::scoped_lock lock(mutex);
        migration_delay_sec = sec; 
    }

    static void set_debug()
    {
        boost::mutex::scoped_lock lock(mutex);
        if (debug <= 0)
        {
            debug = 1;
            boost::thread(boost::bind(&debug_proc));
        }
    }

private:
    typedef boost::aligned_storage<sizeof(pool_type), boost::alignment_of<pool_type>::value> storage_type;
    static storage_type storage[2];  // 存储内存池对象，采用placement new方式分配

    static pool_type& get_malloc_free_pool()
    {
        static bool f = false;
        return get_pool(malloc_free_index, f);
    }

    static pool_type& get_free_only_pool()
    {
        static bool f = false;
        return get_pool(free_only_index, f);
    }


    static pool_type& get_pool(int index, bool & f)
    {
        assert(index < 2);

        if(!f)
        {
            // This code *must* be called before main() starts, 
            // and when only one thread is executing.
            f = true;
            new (&storage[index]) pool_type;
        }

        // The following line does nothing else than force the instantiation
        //  of singleton<T>::create_object, whose constructor is
        //  called before main() begins.
        create_object.do_nothing();

        return *static_cast<pool_type*>(static_cast<void*>(&storage[index]));
    }

    static void clean_pool_proc()
    {

        pool_type & p = get_free_only_pool();
        p.purge_memory();

        std::cout << "purge, max_chunk_num/migrating_max_chunk_num:" << p.max_chunk_num << "/" << p.migrating_max_chunk_num
            << " cur_chunk_num/migrating_used_chunk_num:" << p.cur_chunk_num  << "/" << p.migrating_used_chunk_num
            << " migration_prepared_time:" << p.migration_prepared_time
            << " cur_time:" << time(NULL)
            << " migration_delay_sec:" << migration_delay_sec
            << std::endl;

        boost::mutex::scoped_lock lock(mutex);
        p.reset();
        is_migrating = false;
    }

    static void debug_proc()
    {
        while (true)
        {
            do
            {
                boost::mutex::scoped_lock lock(mutex);
                pool_type & p1 = get_free_only_pool();
                pool_type & p2 = get_malloc_free_pool();
                std::cout << "free_only_pool, max_chunk_num/migrating_max_chunk_num:" << p1.max_chunk_num << "/" << p1.migrating_max_chunk_num
                    << " cur_chunk_num/migrating_used_chunk_num:" << p1.cur_chunk_num  << "/" << p1.migrating_used_chunk_num
                    << " migration_prepared_time:" << p1.migration_prepared_time
                    << " cur_time:" << time(NULL)
                    << " migration_delay_sec:" << migration_delay_sec
                    << std::endl;
                std::cout << "malloc_free_pool, max_chunk_num/migrating_max_chunk_num:" << p2.max_chunk_num << "/" << p2.migrating_max_chunk_num
                    << " cur_chunk_num/migrating_used_chunk_num:" << p2.cur_chunk_num  << "/" << p2.migrating_used_chunk_num
                    << " migration_prepared_time:" << p2.migration_prepared_time
                    << " cur_time:" << time(NULL)
                    << " migration_delay_sec:" << migration_delay_sec
                    << std::endl;
            } while(0);
            sleep(5);
        }
    }

    struct object_creator
    {
        object_creator()
        {  // This constructor does nothing more than ensure that instance()
            //  is called before main() begins, thus creating the static
            //  T object before multithreading race issues can come up.
            migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::get_malloc_free_pool();
            migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::get_free_only_pool();
            migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::migration_delay_sec = MigratingDelaySec;
        }
        inline void do_nothing() const
        {
        }
    };

    static object_creator create_object;  // 类静态变量，在main函数之前初始化，以便在main函数开始之前初始化内存池
    static int malloc_free_index;         // 当前提供malloc接口的内存池索引
    static int free_only_index;           // 当前正在迁移的内存池索引，只提供free接口 
    static bool is_migrating;             // 内存池是否正在迁移，true:正在迁移 false:没有迁移
    static boost::mutex mutex;
    static int migration_delay_sec;       // 满足内存池清理条件并持续该时间，才真正清理内存
    static int debug;                     // debug标记，1：打开debug线程；0：不打开
}; // struct migrating_pool


// 定义静态成员变量
template <typename Tag,
    unsigned RequestedSize,
    unsigned MigratingSize,
    unsigned MigratingUsedSize,
    unsigned MigratingDelaySec,
    unsigned NextSize,
    unsigned MaxSize >
int migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::debug = 0;

template <typename Tag,
    unsigned RequestedSize,
    unsigned MigratingSize,
    unsigned MigratingUsedSize,
    unsigned MigratingDelaySec,
    unsigned NextSize,
    unsigned MaxSize >
int migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::malloc_free_index = 0;

template <typename Tag,
    unsigned RequestedSize,
    unsigned MigratingSize,
    unsigned MigratingUsedSize,
    unsigned MigratingDelaySec,
    unsigned NextSize,
    unsigned MaxSize >
int migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::free_only_index = 1;

template <typename Tag,
    unsigned RequestedSize,
    unsigned MigratingSize,
    unsigned MigratingUsedSize,
    unsigned MigratingDelaySec,
    unsigned NextSize,
    unsigned MaxSize >
bool migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::is_migrating = false;

template <typename Tag,
    unsigned RequestedSize,
    unsigned MigratingSize,
    unsigned MigratingUsedSize,
    unsigned MigratingDelaySec,
    unsigned NextSize,
    unsigned MaxSize >
int migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::migration_delay_sec = 60;

template <typename Tag,
    unsigned RequestedSize,
    unsigned MigratingSize,
    unsigned MigratingUsedSize,
    unsigned MigratingDelaySec,
    unsigned NextSize,
    unsigned MaxSize >
boost::mutex migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::mutex;

template <typename Tag,
    unsigned RequestedSize,
    unsigned MigratingSize,
    unsigned MigratingUsedSize,
    unsigned MigratingDelaySec,
    unsigned NextSize,
    unsigned MaxSize >
typename migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::storage_type migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::storage[2];

template <typename Tag,
    unsigned RequestedSize,
    unsigned MigratingSize,
    unsigned MigratingUsedSize,
    unsigned MigratingDelaySec,
    unsigned NextSize,
    unsigned MaxSize >
typename migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::object_creator migrating_pool<Tag, RequestedSize, MigratingSize, MigratingUsedSize, MigratingDelaySec, NextSize, MaxSize>::create_object;

#endif
