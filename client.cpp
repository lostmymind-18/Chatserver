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
void SIGUSER1_handler(int);
void SIGUSER2_handler(int);

struct Message{
    char mess[200];
};

struct Info{
    char name[50];
    unsigned int room;
    pid_t pid;
    bool complete = false;
};

Info info;
bool inroom=false;
//Connect to server
int   ShmID;
pid_t *ShmPtr;

//Send information to sever
int ShmID1;
Info *ShmPtr1;

//Send/receive messsages
int ShmID2;
void *ShmPtr2 = NULL;
int main(void)
{
     pid_t   pid_server;
     key_t MyKey;
     
     char c;
     std::string line;
     int   i;
     info.pid=getpid();
     std::cout<<"My pid: "<<getpid()<<'\n';
     MyKey   = ftok(".", 's');          /* obtain the shared memory */
     ShmID   = shmget(MyKey, sizeof(pid_t), 0666);
     ShmPtr  = (pid_t *) shmat(ShmID, NULL, 0);
     pid_server     = *ShmPtr;                 /* get process-a's ID       */
     shmdt(ShmPtr);                     /* detach shared memory     */
     //Send signal to the Server
     kill(pid_server, SIGUSR1);
     if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
        printf("SIGINT install error\n");
        exit(1);
     }
     if (signal(SIGUSR1,SIGUSER1_handler)==SIG_ERR)
     {
         printf("SIGUSER1 install error\n");
         exit(1);
     }

     if(signal(SIGUSR2,SIGUSER2_handler)==SIG_ERR)
     {
         printf("SIGUSR2 install error\n");
         exit(1);
     }
     while (1) {
        if(inroom && !ShmPtr2){
            MyKey=ftok(".",info.room);
            std::cout<<"Common key room: "<<MyKey<<'\n';
            ShmID2 = shmget(MyKey,sizeof(Message),0666);
            ShmPtr2 = (void*)shmat(ShmID,NULL,0);
            while((int*)ShmPtr2==(int*)-1){
                ShmID2=shmget(MyKey,sizeof(Message),0666);
                ShmPtr2=(void*)shmat(ShmID2,NULL,0);
                std::cout<<"Waiting for room\n";
            }
        }
        else if(inroom && ShmPtr2){
            Message newMessage;
            std::cin>>newMessage.mess;
            *(Message*)ShmPtr2 = newMessage;
            kill(pid_server,SIGUSR2);
            std::cout<<"Message send: "<<((Message*)ShmPtr2)->mess;
        }
     }
}

void SIGUSER1_handler(int sig)
{
    std::cout<<"Enter your name: \n";
    std::cin>>info.name;
    std::cout<<"Enter room number: \n";
    std::cin>>info.room;
    key_t key=ftok(".",info.pid);
    std::cout<<"common key: "<<key<<'\n';
    ShmID1=shmget(key,sizeof(Info),IPC_CREAT|0666);
    ShmPtr1=(Info*)shmat(ShmID1,NULL,0);
    *ShmPtr1=info;
    ShmPtr1->complete=true;
    std::cout<<"Information entering complete!\n";
}

void SIGUSER2_handler(int sig)
{
    if(!inroom)
    {
        std::cout<<"Hi, "<<info.name<<", now you are in the chat room, have fun!\n";
        inroom = true;
    }
    else{
        std::cout<<*(char*)ShmPtr2<<'\n';
    }
}

void SIGINT_handler(int sig)
{
    std::cout<<"Terminal...\n";
    exit(0);
}