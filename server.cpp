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
#include <thread>


struct Info{
    char name[50];
    unsigned int room;
    pid_t pid;
    bool complete = false;
};


struct Client{
    //Id of client
    std::string id;
    //List of rooms this client's in
    std::vector<unsigned int> roomList;
    friend class Server;
    friend class Room;
};

class Room{
    //Number of clients
    std::vector<Client> clientList;
    //Shared memory pointer
    void* shmptr;
    //Update shared memory
    void set();
    //Get message from one of the clients
    void get();
    //Broadcast to all clients
    void broadcast();    
    friend class Server;
};
//Using Singleton design pattern
class Server{
    bool running;
    pid_t pid;
    key_t key;
    int ShmId;
    pid_t *ShmPtr;
    //Terminate server
    //Regist signal handler
    void registClient(int);
    //List of rooms the server has
    std::vector<Room> room_list;
    //List of people are in rooms of the server
    std::vector<Client> client_list;
    //Client register
    void registClient(std::string id, int roomNumb);
    //Client unregist
    void unregistClient(Client& client);
    //Client listner
    void listenClient();
    //Room create
    void createRoom();
    //Room delete
    void deleteRoom();
    //Room run
    void runRoom(Room&);
    public:
        Server();
        ~Server();
        int run();
        void stop();
};

Server* server;

static void retriveClientInfo(int sig, siginfo_t* siginfo, void* context)
{
    if(sig==SIGINT)
    {
        pid_t sender_pid=siginfo->si_pid;
        std::cout<<"Someone wants to joint!"<<sender_pid<<'\n';
        kill(sender_pid,SIGINT);
        Info info;
        key_t key = ftok(".",sender_pid);
        std::cout<<"key: "<<key<<'\n';
        int ShmID = shmget(key, sizeof(Info), 0666); 
        void* ShmPtr = (void*)shmat(ShmID,NULL,0);
        while((int*)ShmPtr==(int*)-1){
            ShmID=shmget(key,sizeof(Info),0666);
            ShmPtr = (void*)shmat(ShmID,NULL,0);
        }
        while(!((Info*)ShmPtr)->complete){}
        std::cout<<"Name: "<<((Info*)ShmPtr)->name<<'\n';
        std::cout<<"Room: "<<((Info*)ShmPtr)->room<<'\n';
        std::cout<<"pid: "<<((Info*)ShmPtr)->pid<<'\n';
    }

    else if(sig==SIGQUIT&&siginfo->si_pid==0)
    {
        std::cout<<"Quit..."<<'\n';
        delete server;
        exit(0);
    }
}

Server::Server()
{
    this->running=false;
    this->pid=getpid();
    key=ftok(".",'s');
    ShmId=shmget(key,sizeof(pid_t),IPC_CREAT|0666);
    ShmPtr=(pid_t*)shmat(ShmId,NULL,0);
    *ShmPtr=pid;
    std::cout<<this->pid<<'\n';
}

Server::~Server()
{
    std::cout<<"Get out\n";
}

void Server::stop(){
    this->running=false;
}

int Server::run()
{
    this->running=true;
    struct sigaction siga;
    siga.sa_sigaction=retriveClientInfo;
    siga.sa_flags=SA_SIGINFO;
    if(sigaction(SIGINT,&siga,NULL)!=0)
    {
        printf("Error sigaction()");
        return errno;
    }

    if(sigaction(SIGQUIT,&siga,NULL)!=0)
    {
        printf("Error sigaction()");
        return errno;
    }

    while(this->running){}
    //Create a specific thread for listening new client
    
    //Create list of thread correspond to each room in the server
}


int main()
{
    server = new Server();
    server->run();
}