/*Author: Tran Manh Dung*/

#include "type.h"

void SIGINT_handler(int);
void SIGQUIT_handler(int);
void SIGUSER1_handler(int);
void SIGUSER2_handler(int);

Info info;
bool inroom=false;
//Connect to server
int   ShmID;
Serverinfo *ShmPtr;

//Send information to sever
int ShmID1;
Info *ShmPtr1;
pid_t   pid_server;
//Send receive messsages
int ShmID2;
void *ShmPtr2 = NULL;
int main(void)
{
     key_t MyKey;
     char c;
     std::string line;
     int   i;
     info.pid=getpid();
     std::cout<<"My pid: "<<getpid()<<'\n';
     MyKey   = ftok(".",'s');          /* obtain the shared memory */
     ShmID   = shmget(MyKey, sizeof(Serverinfo), 0666);
     std::cout<<"Shared memory server id: "<<ShmID<<'\n';
     ShmPtr  = (Serverinfo *) shmat(ShmID, NULL, 0);
     pid_server     = ShmPtr->pid;                 /* get process-a's ID       */
     std::cout<<"Server's pid: "<<pid_server<<'\n';
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
            key_t myKey=ftok(".",info.room);
            std::cout<<"Common key room: "<<myKey<<'\n';
            ShmID2 = shmget(myKey,sizeof(Message),0666);
            if(ShmID2==-1)
            {
                std::cout<<"Error\n";
                exit(-1);
            }
            ShmPtr2 = (void*)shmat(ShmID,NULL,0);
            while((int*)ShmPtr2==(int*)-1){
                ShmID2=shmget(myKey,sizeof(Message),0666);
                ShmPtr2=(void*)shmat(ShmID2,NULL,0);
            }
        }
        else if(inroom && ShmPtr2){
            Message newMessage;
            fgets(newMessage.mess,100,stdin);
            memcpy(((Message*)ShmPtr2)->name,info.name,strlen(info.name)+1);
            memcpy(((Message*)ShmPtr2)->mess,newMessage.mess,strlen(newMessage.mess)+1);
            kill(pid_server,SIGUSR2);
            std::cout<<"You: "<<((Message*)ShmPtr2)->mess<<'\n';
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
    ShmID1=shmget(key,sizeof(Info),IPC_CREAT|0666);
    ShmPtr1=(Info*)shmat(ShmID1,NULL,0);
    *ShmPtr1=info;
    ShmPtr1->complete=true;
    std::cout<<"Information entering complete!\n";
    std::cout<<"If you wait over 2 seconds but not in the chat room yet, you should Ctr+C and try again\n";
    shmdt(ShmPtr1);
}

void SIGUSER2_handler(int sig)
{
    if(!inroom)
    {
        std::cout<<"Hi, "<<info.name<<", now you are in the chat room, have fun!\n";
        inroom = true;
    }
    else{
        std::cout<<((Message*)ShmPtr2)->name<<": "<<((Message*)ShmPtr2)->mess<<'\n';
    }
}

void SIGINT_handler(int sig)
{
    std::cout<<"Terminal...\n";
    shmdt(ShmPtr2);
    kill(pid_server,SIGHUP);
    exit(0);
}