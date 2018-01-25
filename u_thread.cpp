/*******************************************************
 * Created by @firejq on 1/14/2018.
 *******************************************************/

#include "u_thread.h"

/*******************************************************
 * 定义全局变量
 *******************************************************/
thread_node * cur_node;
jmp_buf main_jbuf;
sigset_t oldmask;
thread_list * u_threads_ready;
thread_list * u_threads_block;


/*******************************************************
 * 定义线程库函数实现
 *******************************************************/

/**
 * 构造函数：初始化线程库
 */
u_thread::u_thread():u_init_flag(0), u_threads_total(0), u_error_num(ERROR_NOERROR) {

    /* 若已初始化完毕，不再重复执行 */
    if(this->u_init_flag == 1) {
        this->u_error_num = ERROR_DUPLINIT;
        return;
    }

    /* 初始化 ready 和 block 线程队列 */
    u_threads_ready = (thread_list *)malloc(sizeof(thread_list));
    u_threads_block = (thread_list *)malloc(sizeof(thread_list));
    if (u_threads_ready == NULL || u_threads_block == NULL) {
        this->u_error_num = ERROR_MEMALLOC;
        return;
    }
    this->mask();
    u_threads_ready->head = u_threads_ready->tail = NULL;
    u_threads_block->head = u_threads_block->tail = NULL;
    u_threads_ready->lenght = u_threads_block->lenght = 0;
    this->unmask();

    /* 标识当前线程库已初始化完毕 */
    this->u_init_flag = 1;

}

/**
 * 在线程库中创建一个新的线程
 * @param proc
 * @param param
 * @return
 */
int u_thread::u_thread_create(void (*procedure)(int), int param, int priority) {

    /* 若未初始化，保存错误码后退出 */
    if (this->u_init_flag == 0) {
        this->u_error_num = ERROR_NOTINIT;
        return -1;
    }

    /* 若创建新线程后线程库中的线程将超过限制，则保存错误码后退出 */
    if (this->u_threads_total == THREADSMAX) {
        this->u_error_num = ERROR_THREADMAX;
        return -1;
    }

    /* 创建新线程信息 */
    thread_context * new_info = (thread_context *)malloc(sizeof(thread_context));

    int jmp_res = sigsetjmp(new_info->jbuf, 1);

    if (jmp_res == 1) {
        /* 由调度器选择此线程时，执行线程内的用户函数 */
        std::cout << "开始执行线程-tid:【"
                  << cur_node->info->tid
                  << "】"
                  << std::endl;
        gettimeofday(&(cur_node->info->timeBeforeSched), NULL);
        cur_node->info->state = RUN;
        (*(cur_node->info->procedure))(cur_node->info->param);
        cur_node->info->bursts ++;
        /* 用户函数执行完毕后，切换线程状态并返回主线程 */
        cur_node->info->state = READY;
        gettimeofday(&(cur_node->info->timeAfterSched), NULL);
        cur_node->info->run_time = cur_node->info->timeAfterSched.tv_usec
                                   - cur_node->info->timeBeforeSched.tv_usec;
        std::cout << "线程-tid:【"
                  << cur_node->info->tid
                  << "】执行完毕，共运行"
                  << cur_node->info->run_time
                  << "ms"
                  << std::endl;
        siglongjmp(main_jbuf, 1);

    } else if (jmp_res == 0) {
        /* 初始化新线程节点的信息 */
        new_info->tid = ++(this->u_threads_total);
        /* 计算堆栈内存空间并存储 */
        new_info->sp = (char *)malloc(STACKSIZE * sizeof(char))
                       + STACKSIZE - sizeof(long int);
        /* 新线程的堆栈大小 */
        long int new_base = (long int)new_info->sp;
        (new_info->jbuf->__jmpbuf)[24] = new_base;
        (new_info->jbuf->__jmpbuf)[64] = new_base;
        new_info->jbuf->__mask_was_saved = 1;

        new_info->procedure = procedure;
        new_info->param = param;
        new_info->state = READY;
        new_info->bursts = 0;
        new_info->vq = VQ_BASE;
        new_info->priority = priority;
        // gettimeofday(&(new_info->timeStart), NULL);
        // new_info.status.total_time = 0;

        /* 创建新线程节点，为新的线程节点分配内存空间 */
        thread_node * new_node = (thread_node *)malloc(sizeof(thread_node));
        /* 若内存分配失败，保存错误码后直接退出 */
        if(new_node == NULL) {
            this->u_error_num = ERROR_MEMALLOC;
            return -1;
        }
        new_node->info = new_info;
        new_node->next = NULL;
        /* 将新的线程节点 push 到 ready 队列中 */
        if (u_threads_ready->head == NULL) {
            u_threads_ready->head = u_threads_ready->tail = new_node;
        } else {
            u_threads_ready->tail->next = new_node;
            u_threads_ready->tail = new_node;
        }
        u_threads_ready->lenght ++;

        /* 返回新创建线程的唯一 tid */
        return new_info->tid;
    }
}

/**
 * 开始线程库的调度执行，当线程库中最后一个线程退出时该函数才返回
 * @param scheAlgorithm
 * @return
 */
int u_thread::u_thread_start(sche_algorithm scheAlgorithm) {

    /* 若未初始化，保存错误码后退出 */
    if (this->u_init_flag == 0) {
        this->u_error_num = ERROR_NOTINIT;
        return -1;
    }

    /* 若就绪队列为空，保存错误码后返回 */
    if (u_threads_ready->lenght == 0) {
        this->u_error_num = ERROR_THREADNULL;
        return -1;
    }

    this->scheAlgorithm = scheAlgorithm;
    /* 按参数需求使用不同的调度算法 */
    switch (this->scheAlgorithm) {
        case FCFS: {
            /* 使用先来先服务调度算法 */
            this->sche_FCFS();
            break;
        }
        case RR: {
            /* 使用轮转调度算法 */
            this->sche_RR();
            break;
        }
        case HP: {
            /* 使用优先级调度算法 */
            this->sche_HP();
            break;
        }
        default: {
            /* 默认情况下使用先来先服务调度算法 */
            this->sche_FCFS();
            break;
        }
    }
    return 0;
}

/**
 * 先来先服务调度
 * @return
 */
int u_thread::sche_FCFS() {

    // std::cout << "here is fcfs" << std::endl;

    /* 保存主线程，以供调度器返回 */
    int jmp_res = sigsetjmp(main_jbuf, 1);

    if (jmp_res == 0) {
        /* 保存主线程后，取就绪队列的头节点线程开始执行 */
        cur_node = u_threads_ready->head;
        siglongjmp(u_threads_ready->head->info->jbuf, 1);

    } else if (jmp_res == 1) {
        /* 计算运行时间 */
        gettimeofday(&(cur_node->info->timeAfterSched), NULL);
        cur_node->info->run_time = cur_node->info->timeAfterSched.tv_usec
                                   - cur_node->info->timeBeforeSched.tv_usec;
        thread_node * finished_thread = u_threads_ready->head;
        if (u_threads_ready->head->next == NULL) {
            /* 若新线程执行完毕返回调度器后，若已没有下一个线程节点，则释放内存后退出调度器 */
            free(finished_thread);
            u_threads_ready->lenght --;
            return 0;
        } else {
            /* 若新线程执行完毕返回调度器后，若还有下一个线程节点，
             * 则先将头指针指向下一个节点，释放已完成线程的内存后，再执行下一个线程 */
            u_threads_ready->head = u_threads_ready->head->next;
            free(finished_thread);
            u_threads_ready->lenght --;

            /* 取就绪队列的头节点线程开始执行 */
            cur_node = u_threads_ready->head;
            siglongjmp(cur_node->info->jbuf, 1);
        }
    }
    return 0;
}

/**
 * 轮转调度
 * @return
 */
int u_thread::sche_RR() {

    pid_t pid = fork();
    if (pid < 0) {
        this->u_error_num = ERROR_SIGNALGENERATE;
        return -1;
    } else if (pid == 0) {
        /* 创建子进程，定期向父进程发射用户信号，模拟时钟滴答 */
        while (true) {
            kill(getppid(), SIGUSR1);
            sleep(1);
        }
    } else if (pid > 0) {
        /* 父进程注册信号响应函数 */
        if (signal(SIGUSR1, sche_RR_check) == SIG_ERR) {
            this->u_error_num = ERROR_SIGNALHANDLER;
            return -1;
        }
        /* 保存主线程环境，以供返回 */
        int jmp_res = sigsetjmp(main_jbuf, 1);

        if (jmp_res == 0) {
            /* 取 ready 队列头节点开始执行 */
            cur_node = u_threads_ready->head;
            siglongjmp(cur_node->info->jbuf, 1);
        } else if (jmp_res == 1) {
            thread_node * finished_node = u_threads_ready->head;
            if (u_threads_ready->head->next == NULL) {
                /* 若新线程执行完毕返回调度器后，若已没有下一个线程节点，则释放内存后退出调度器 */
                free(finished_node);
                u_threads_ready->lenght --;
                /* 杀死信号发射进程 */
                kill(getpid(), SIGTERM);
                // TODO 取消注册响应函数
                return 0;
            } else {
                /* 若新线程执行完毕返回调度器后，若还有下一个线程节点，
                 * 则先将头指针指向下一个节点，释放已完成线程的内存后，再执行下一个线程 */
                u_threads_ready->head = u_threads_ready->head->next;
                free(finished_node);
                u_threads_ready->lenght --;
                /* 取 ready 队列头节点进行执行 */
                cur_node = u_threads_ready->head;
                siglongjmp(cur_node->info->jbuf, 1);
            }
        }
    }
}

/**
 * 轮转调度的辅助函数，响应每次信号
 * @param signal
 */
void u_thread::sche_RR_check(int signal) {

    gettimeofday(&(cur_node->info->timeAfterSched), NULL);
    cur_node->info->run_time += cur_node->info->timeAfterSched.tv_usec
                                - cur_node->info->timeBeforeSched.tv_usec;

    // std::cout << cur_node->info->run_time << std::endl;

    if (cur_node->info->run_time >= cur_node->info->vq * 1000) {
        std::cout << cur_node->info->run_time << "【over，移至队尾】" << std::endl;
        /* 运行时间已超过时间片，将当前线程移至队尾并为其分配新的时间片，头指针指向下一个线程节点 */
        cur_node->info->vq *= 2;
        u_threads_ready->tail->next = u_threads_ready->head;
        u_threads_ready->tail = u_threads_ready->head;
        u_threads_ready->head = u_threads_ready->head->next;
        u_threads_ready->tail->next = NULL;
        /* 取 ready 队列的头节点进行运行 */
        cur_node = u_threads_ready->head;
        siglongjmp(cur_node->info->jbuf, 1);

    } else {
        /* 运行时间还不足时间片，让线程继续运行 */
        std::cout << cur_node->info->run_time << " 【not over】" << std::endl;
    }
}

/**
 * 优先级调度
 * @return
 */
int u_thread::sche_HP() {

    pid_t pid = fork();
    if (pid < 0) {
        this->u_error_num = ERROR_SIGNALGENERATE;
        return -1;
    } else if (pid == 0) {
        /* 创建子进程，定期向父进程发射用户信号，模拟时钟滴答 */
        while (true) {
            kill(getppid(), SIGUSR1);
            sleep(1);
        }
    } else if (pid > 0) {
        /* 父进程注册信号响应函数 */
        if (signal(SIGUSR1, sche_HP_check) == SIG_ERR) {
            this->u_error_num = ERROR_SIGNALHANDLER;
            return -1;
        }
        /* 保存主线程环境，以供返回 */
        int jmp_res = sigsetjmp(main_jbuf, 1);

        if (jmp_res == 0) {
            /* 取 ready 队列中优先级最高的节点开始执行 */
            thread_node * tmp = u_threads_ready->head;
            thread_node * max_node = tmp;
            while ((tmp = tmp->next) != NULL) {
                if (tmp->info->priority > max_node->info->priority) {
                    max_node = tmp;
                }
            }
            cur_node = max_node;
            siglongjmp(cur_node->info->jbuf, 1);
        } else if (jmp_res == 1) {
            thread_node * finished_node = u_threads_ready->head;
            if (u_threads_ready->head->next == NULL) {
                /* 若新线程执行完毕返回调度器后，若已没有下一个线程节点，则释放内存后退出调度器 */
                free(finished_node);
                u_threads_ready->lenght --;
                /* 杀死信号发射进程 */
                kill(getpid(), SIGTERM);
                // TODO 取消注册响应函数
                return 0;
            } else {
                /* 若新线程执行完毕返回调度器后，若还有下一个线程节点，
                 * 则先将头指针指向下一个节点，释放已完成线程的内存后，再执行下一个线程 */
                u_threads_ready->head = u_threads_ready->head->next;
                free(finished_node);
                u_threads_ready->lenght --;
                /* 取 ready 队列中优先级最高的节点开始执行 */
                thread_node * tmp = u_threads_ready->head;
                thread_node * max_node = tmp;
                while ((tmp = tmp->next) != NULL) {
                    if (tmp->info->priority > max_node->info->priority) {
                        max_node = tmp;
                    }
                }
                cur_node = max_node;
                siglongjmp(cur_node->info->jbuf, 1);
            }
        }
    }
    return 0;
}

/**
 * 优先级调度的辅助函数，响应每次信号
 * @param signal
 */
void u_thread::sche_HP_check(int signal) {

    gettimeofday(&(cur_node->info->timeAfterSched), NULL);
    cur_node->info->run_time += cur_node->info->timeAfterSched.tv_usec
                                - cur_node->info->timeBeforeSched.tv_usec;

    // std::cout << cur_node->info->run_time << std::endl;

    if (cur_node->info->run_time >= cur_node->info->vq * 1000) {
        std::cout << cur_node->info->run_time << "【over，移至队尾】" << std::endl;
        /* 运行时间已超过时间片，为当前线程分配新的时间片，然后降低优先级 */
        cur_node->info->vq *= 2;
        cur_node->info->priority /= 2;
        /* 取 ready 队列中优先级最高的节点开始执行 */
        thread_node * tmp = u_threads_ready->head;
        thread_node * max_node = tmp;
        while ((tmp = tmp->next) != NULL) {
            if (tmp->info->priority > max_node->info->priority) {
                max_node = tmp;
            }
        }
        cur_node = max_node;
        siglongjmp(cur_node->info->jbuf, 1);

    } else {
        /* 运行时间还不足时间片，让线程继续运行 */
        std::cout << cur_node->info->run_time << " 【not over】" << std::endl;
    }

}


/**
 * 执行线程的主动让步
 * @return
 */
int u_thread::u_thread_yeild() {
    std::cout << "线程【" << u_threads_ready->head->info->tid << "】主动让步" <<std::endl;

    /* 将 ready 队列中的头节点移至队尾，并将头指针指向下一个节点 */
    u_threads_ready->tail->next = u_threads_ready->head;
    u_threads_ready->tail = u_threads_ready->tail->next;
    u_threads_ready->head = u_threads_ready->head->next;
    u_threads_ready->tail->next = NULL;

    /* 取 ready 队列中的头节点线程执行 */
    cur_node = u_threads_ready->head;
    siglongjmp(cur_node->info->jbuf, 1);
}

/**
 * 执行线程的主动退出
 * @return
 */
int u_thread::u_thread_exit() {

    thread_node * kill_node = cur_node;
    switch (this->scheAlgorithm) {
        case FCFS:
            if (cur_node->next != NULL) {
                cur_node = cur_node->next;
            }
            break;
        case RR:
            if (cur_node->next != NULL) {
                cur_node = cur_node->next;
                gettimeofday(&(cur_node->info->timeAfterSched), NULL);
                cur_node->info->run_time += cur_node->info->timeAfterSched.tv_usec
                                            - cur_node->info->timeBeforeSched.tv_usec;
                if (cur_node->info->run_time >= cur_node->info->vq * 1000) {
                    std::cout << cur_node->info->run_time << "【over，移至队尾】" << std::endl;
                    /* 运行时间已超过时间片，为当前线程分配新的时间片 */
                    cur_node->info->vq *= 2;
                    /* 取 ready 队列的头节点进行运行 */
                    cur_node = u_threads_ready->head;
                    siglongjmp(cur_node->info->jbuf, 1);
                }
            }
            break;
        case HP:
            if (cur_node->next != NULL) {
                cur_node = cur_node->next;
                gettimeofday(&(cur_node->info->timeAfterSched), NULL);
                cur_node->info->run_time += cur_node->info->timeAfterSched.tv_usec
                                            - cur_node->info->timeBeforeSched.tv_usec;

                if (cur_node->info->run_time >= cur_node->info->vq * 1000) {
                    std::cout << cur_node->info->run_time << "【over，移至队尾】" << std::endl;
                    /* 运行时间已超过时间片，为当前线程分配新的时间片，然后降低优先级 */
                    cur_node->info->vq *= 2;
                    cur_node->info->priority /= 2;
                    /* 取 ready 队列中优先级最高的节点开始执行 */
                    thread_node * tmp = u_threads_ready->head;
                    thread_node * max_node = tmp;
                    while ((tmp = tmp->next) != NULL) {
                        if (tmp->info->priority > max_node->info->priority) {
                            max_node = tmp;
                        }
                    }
                    cur_node = max_node;
                    siglongjmp(cur_node->info->jbuf, 1);

                } else {
                    /* 运行时间还不足时间片，让线程继续运行 */
                    std::cout << cur_node->info->run_time << " 【not over】" << std::endl;
                }
            }
            break;
        default:
            exit(1);
    }
    std::cout << "线程【" << kill_node->info->tid << "】已主动退出" << std::endl;
    free(kill_node);
    return 0;
}

/**
 * 删除线程库中的线程
 * @return
 */
int u_thread::u_thread_delete(int tid) {
    thread_node * tmp = u_threads_ready->head;
    thread_node * next = u_threads_ready->head;
    while (next->next != NULL) {
        if (next == u_threads_ready->head) {
            if (next->info->tid == tid) {
                u_threads_ready->head = next->next;
                free(next);
                break;
            }
            next = next->next;

        } else {
            if (next->info->tid == tid) {
                tmp->next = next->next;
                free(next);
                break;
            }
            tmp = next;
            next = next->next;
        }
    }
    return 0;
}

/**
 * 设置屏蔽信号
 */
void u_thread::mask() {
    sigset_t newmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &newmask, &oldmask);
}

/**
 * 取消屏蔽信号
 */
void u_thread::unmask() {
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
}


/**
 * 输出当前线程库的错误信息
 */
void u_thread::u_thread_error() {
    /* 输出错误信息 */
    switch(this->u_error_num){
        case ERROR_NOERROR:
            std::cout << "线程库目前没有已知错误！" << std::endl;
            break;
        case ERROR_DUPLINIT:
            std::cout << "线程库重复初始化！" << std::endl;
            break;
        case ERROR_MEMALLOC:
            std::cout << "线程库内存分配错误！" << std::endl;
            break;
        case ERROR_NOTINIT:
            std::cout << "线程库未初始化！" << std::endl;
            break;
        case ERROR_THREADMAX:
            std::cout << "线程库内线程数量超过最大限制！" << std::endl;
            break;
        case ERROR_THREADNULL:
            std::cout << "线程库内线程数量为空！" << std::endl;
            break;

        default:
            std::cout << "未知错误！" << std::endl;
            break;
    }
}


