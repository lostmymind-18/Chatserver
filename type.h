/*Author: Tran Manh Dung*/
#ifndef TYPE_H_
#define TYPE_H_
#include  <string>
#include  <vector>
#include  <stdio.h>
#include  <sys/types.h>
#include  <signal.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <unistd.h>
#include  <iostream>
#include  <errno.h>
#include  <pthread.h>
#include  <thread>
#include  <string.h>

struct Serverinfo{
    pid_t pid;
};

struct Message{
    pid_t pid = 5;
    char mess[100];
    char name[50];
};

struct Info{
    char name[20];
    unsigned int room;
    pid_t pid;
    bool complete = false;
};

#endif