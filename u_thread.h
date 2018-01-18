/*******************************************************
 * Created by @firejq on 1/14/2018.
 *******************************************************/


#ifndef USERLEVELTHREADSLIB_U_THREAD_H
#define USERLEVELTHREADSLIB_U_THREAD_H

/*******************************************************
 * 依赖的头文件
 *******************************************************/
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <csetjmp>
#include <csignal>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <wait.h>

using namespace std;

/*******************************************************
 * 定义常量值
 *******************************************************/
#define THREADSMAX 50                   // 定义最大线程数
#define STACKSIZE 4096		            // 定义新创建线程的默认 stack size
#define VQ_BASE 2                       // 定义默认时间片大小：2s

#define ERROR_NOERROR 0                 // 错误码：当前没有错误
#define ERROR_DUPLINIT 1                // 错误码：重复初始化
#define ERROR_MEMALLOC 2                // 错误码：内存分配失败
#define ERROR_NOTINIT 3                 // 错误码：未执行初始化
#define ERROR_THREADMAX 4               // 错误码：线程数超过最大限制
#define ERROR_THREADNULL 5              // 错误码：线程库为空
#define ERROR_SIGNALHANDLER 6           // 错误码：信号处理函数错误
#define ERROR_SIGNALGENERATE 7          // 错误码：信号发射器错误


/*******************************************************
 * 定义数据结构
 *******************************************************/
/* 线程状态枚举 */
enum state {
    RUN,                                // 运行态
    BLOCKED,                            // 阻塞态
    READY                               // 就绪态
};

/* 可用调度算法枚举 */
enum sche_algorithm {
    FCFS,                               // First-come First-Service 先来先服务调度算法
    RR,                                 // Round Robin 轮转调度算法
    HP                                  // High Priority 高优先级调度算法
};


/* 保存一个用户级线程的所有信息 */
struct thread_context {
    int tid;			                // 每个线程的唯一 id
    jmp_buf jbuf;		                // 线程当前的寄存器信息
    char * sp;			                // 线程当前的堆栈信息
    void (* procedure)(int);	        // 线程内执行的函数指针
    int param;			                // 线程内执行的函数参数
    enum state state;	                // 线程当前的状态
    long total_time; 	                // 线程从创建到退出的累计时间(in microseconds)
    long run_time; 		                // 线程处于 RUN 状态的累计时间(in microsecond)
    int bursts;   		                // 线程被调度器选中的累计次数
    int vq;				                // virtual quantum remains
    int priority;                       // 进程优先级
    timeval timeBeforeSched;            // 线程调度前的时间点，用于计算 run_time
    timeval timeAfterSched;             // 线程调度的时间点，用于计算 run_time
//    timeval timeStart;	            // start time in uthreads_spawn()
//    timeval timeEnd;		            // end time in uthreads_exit()
};

/* 线程队列中的节点结构 */
struct thread_node {
    thread_context * info;		        // 保存该节点的线程的所有信息
    thread_node * next;			        // 指向下一个线程节点的指针
};

/* 线程队列 */
struct thread_list {
    thread_node * head;			        // 线程队列的首部节点指针
    thread_node * tail;			        // 线程队列的尾部节点指针
    int lenght;					        // 线程队列的节点数量
};


/*******************************************************
 * 定义全局变量
 *******************************************************/
extern thread_node * cur_node;          // 当前运行态线程节点
extern jmp_buf main_jbuf;               // 存储 main 函数的执行环境，以在所有线程退出后返回 main 函数
extern sigset_t oldmask;		        // 用于屏蔽信号
extern thread_list * u_threads_ready;	// 就绪态线程队列
extern thread_list * u_threads_block;   // 阻塞态线程队列
/*******************************************************
 * 定义用户级线程库主类
 *******************************************************/
class u_thread {
private:
    int u_init_flag;                    // 线程库初始化标识： 1 表示已初始化，0 表示未初始化
    int u_threads_total;                // 线程库创建的线程的累计数量，用于生成 tid
    int u_error_num;                    // 线程库错误码， 0 表示无错误
    sche_algorithm scheAlgorithm;       // 线程库调度算法
    /**
     * 设置屏蔽信号
     */
    void mask();

    /**
     * 取消屏蔽信号
     */
    void unmask();

    /**
     * 轮转调度的辅助函数，响应每次信号
     * @param signal
     */
    static void sche_RR_check(int signal);

    /**
     * 轮转调度的辅助函数，响应每次信号
     * @param signal
     */
    static void sche_HP_check(int signal);

public:
    /**
     * 构造函数：初始化线程库
     */
    u_thread();

    /**
     * 在线程库中创建一个新的线程
     * @param proc
     * @param param
     * @return
     */
    int u_thread_create(void (*proc)(int), int param, int priority);

    /**
     * 开始线程库的调度执行，当线程库中最后一个线程退出时该函数才返回
     * @param scheAlgorithm
     * @return
     */
    int u_thread_start(sche_algorithm scheAlgorithm);

    /**
     * 先来先服务调度
     * @return
     */
    int sche_FCFS();

    /**
     * 轮转调度
     * @return
     */
    int sche_RR();

    /**
     * 优先级调度
     * @return
     */
    int sche_HP();

    /**
     * 执行线程的主动让步
     * @return
     */
    int u_thread_yeild();

    /**
     * 执行线程的主动退出
     * @return
     */
    int u_thread_exit();

    /**
     * 删除线程库中的线程
     * @return
     */
    int u_thread_delete(int tid);

    /**
     * 输出当前线程库的错误信息
     */
    void u_thread_error();
};


#endif //USERLEVELTHREADSLIB_U_THREAD_H
