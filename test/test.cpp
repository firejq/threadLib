/*******************************************************
 * Created by @firejq on 1/14/2018.
 *******************************************************/
#include "../u_thread.h"

u_thread * uThread = new u_thread();
int a = 0;
int b = 0;
int c = 0;

void f(int param) {
    if (a++==0) {
        uThread->u_thread_yeild();
    }
    std::cout << "this is thread1" << std::endl;
}

void g(int param) {
    std::cout << "this is thread2" << std::endl;
}

void h(int param) {
    std::cout << "this is thread3" << std::endl;
}

void i(int param) {
    std::cout << "begin thread4" << std::endl;
    if (a++==0) {
        for (int i = 0; i < 4; i ++) {
            sleep(1);
        }
    }
    std::cout << "end thread4" << std::endl;

}

void j(int param) {
    std::cout << "begin thread5" << std::endl;
    if (b++==0) {
        for (int i = 0; i < 8; i ++) {
            sleep(1);
        }
    }
    std::cout << "end thread5" << std::endl;
}

void k(int param) {
    std::cout << "begin thread6" << std::endl;
    if (c++==0) {
        for (int i = 0; i < 8; i ++) {
            sleep(1);
        }
    }
    std::cout << "end thread6" << std::endl;
}


int main() {
    sche_algorithm scheAlgorithm = RR;

    switch (scheAlgorithm) {
        case FCFS:
            uThread->u_thread_create(f, 10, 0);
            uThread->u_thread_create(g, 20, 0);
            uThread->u_thread_create(h, 30, 0);

            uThread->u_thread_start(FCFS);
            break;
        case RR:
            uThread->u_thread_create(i, 10, 0);
            uThread->u_thread_create(j, 20, 0);
            uThread->u_thread_create(k, 30, 0);

            uThread->u_thread_start(RR);
            break;
        case HP:
            uThread->u_thread_create(i, 10, 1);
            uThread->u_thread_create(j, 20, 2);
            uThread->u_thread_create(k, 30, 4);

            uThread->u_thread_start(HP);
            break;
    }
    return 0;


}