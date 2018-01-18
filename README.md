
# user-level-threadlib

一个使用 `c++ 98` 实现的简易用户级线程库.


## Interface

![image](http://otaivnlxc.bkt.clouddn.com/threadlib.png)

## Feature

- 通过 `sigsetjmp` 和 `siglongjmp` 保存寄存器信息进行线程上下文切换；
- 使用 unix 信号在用户态空间模拟时钟滴答；
- 实现以下调度算法：FCFS、RR、HP；


## TODO

- [ ] 加入信号量，实现线程的互斥与顺序保证

## LISCENSE

The user-level-threadlib is under the MIT Liscense.
