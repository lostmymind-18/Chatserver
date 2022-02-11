/* ---------------------------------------------------------------- */
/* PROGRAM  process-b.c:                                            */
/*   This program demonstrates the use of the kill() system call.   */
/* This process reads in commands and sends the corresponding       */
/* to process-a.  Note that process-a must run before process-b for */
/* process-b to retrieve process-a's pid through the shared memory  */
/* segment created by process-a.                                    */
/* ---------------------------------------------------------------- */

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
#include  <cstdio>
void SIGINT_handler(int);
void SIGQUIT_handler(int);
struct Info{
    char name[50];
    unsigned int room;
    pid_t pid;
    bool complete = false;
};

Info info;

//Connect to server
int   ShmID;
pid_t *ShmPtr;

//Send information to sever
int ShmID1;
Info *ShmPtr1;

int main(void)
{
     pid_t   pid;
     key_t MyKey;
     
     char c;
     std::string line;
     int   i;
     std::cout<<"My pid: "<<getpid()<<'\n';
     MyKey   = ftok(".", 's');          /* obtain the shared memory */
     ShmID   = shmget(MyKey, sizeof(pid_t), 0666);
     ShmPtr  = (pid_t *) shmat(ShmID, NULL, 0);
     pid     = *ShmPtr;                 /* get process-a's ID       */
     shmdt(ShmPtr);                     /* detach shared memory     */
     //Send signal to the Server
     kill(pid, SIGINT);
     if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
        printf("SIGINT install error\n");
        exit(1);
     }
     if (signal(SIGQUIT, SIGQUIT_handler) == SIG_ERR) {
        printf("SIGQUIT install error\n");
        exit(2);
     }
     while (1) {
     }
}

void SIGINT_handler(int sig)
{
    signal(sig,SIG_IGN);
    info.pid=getpid();
    std::cout<<"Enter your name: \n";
    std::cin>>info.name;
    std::cout<<"Enter room number: \n";
    std::cin>>info.room;
    key_t key=ftok(".",info.pid);
    std::cout<<"Key: "<<key<<'\n';
    ShmID1=shmget(key,sizeof(Info),IPC_CREAT|0666);
    ShmPtr1=(Info*)shmat(ShmID1,NULL,0);
    *ShmPtr1=info;
    ShmPtr1->complete=true;
}

void  SIGQUIT_handler(int sig)
{
     signal(sig, SIG_IGN);
     printf("From SIGQUIT: just got a %d (SIGQUIT ^\\) signal"
                          " and is about to quit\n", sig);
     shmdt(ShmPtr);
     shmctl(ShmID, IPC_RMID, NULL);
     exit(3);
}
               