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
    pid_t pid;
    Client(){
        this->id = "";
        this->pid = -1;
    }
    Client(std::string id, int roomNumb, pid_t pid):id{id},pid{pid}
    {
        for(std::vector<unsigned int>::iterator it = roomList.begin();it!=roomList.end();it++)
        {
            if(pid == *it)
                return;
        }
        roomList.push_back(roomNumb);
    };
    friend class Server;
    friend class Room;
};

class Room{
    //Number of clients
    unsigned int numb;
    std::vector<Client*> clientList;
    //Shared memory pointer
    void* shmptr;
    //Update shared memory
    void set();
    //Get message from one of the clients
    void get();
    //Broadcast to all clients
    void broadcast();
    //Add new client
    void addClient(Client*);    
    public:
        Room():numb{0},shmptr{nullptr}{};
        Room(unsigned int numb):numb{numb},shmptr{nullptr}{};
    friend class Server;
};

void Room::addClient(Client* client){
    this->clientList.push_back(client);
    std::cout<<"Adding new client complete!\n";
}

//Using Singleton design pattern
class Server{
    bool running;
    pid_t pid;
    key_t key;
    int ShmId;
    pid_t *ShmPtr;
    //List of rooms the server has
    std::vector<Room*> room_list;
    //List of people are in rooms of the server
    std::vector<Client*> client_list;
    //Client register
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
        void registClient(std::string id, int roomNumb,pid_t pid);
        void stop();
};

Server* server;

static void getInfo(int sig, const siginfo_t siginfo, void* context){
    if(sig==SIGINT)
    {
        pid_t sender_pid=siginfo.si_pid;
        std::cout<<"Someone wants to joint!, with pid: "<<sender_pid<<'\n';
        kill(sender_pid,SIGINT);
        Info info;
        key_t key = ftok(".",sender_pid);
        int ShmID = shmget(key, sizeof(Info), 0666); 
        void* ShmPtr = (void*)shmat(ShmID,NULL,0);
        while((int*)ShmPtr==(int*)-1){
            ShmID=shmget(key,sizeof(Info),0666);
            ShmPtr = (void*)shmat(ShmID,NULL,0);
        }
        while(!((Info*)ShmPtr)->complete){}
        std::cout<<"Alright, now regist client\n";
        server->registClient(((Info*)ShmPtr)->name,((Info*)ShmPtr)->room,((Info*)ShmPtr)->pid);
    }

    else if(sig==SIGQUIT&&siginfo.si_pid==0)
    {
        std::cout<<"Quit..."<<'\n';
        delete server;
        exit(0);
    }
}

static void newComeHandler(int sig, siginfo_t* siginfo, void* context)
{
    std::thread my_thread(getInfo,sig,*siginfo,context);   
    my_thread.detach();
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
    /*if(sigaction(SIGINT,&siga,NULL)!=0)
    {
        printf("Error sigaction()");
        return errno;
    }*/
    while(this->running){
        char a = 'x';
        std::cout<<"Enter a: ";
        std::cin>>a;
        if(a=='s')
        {
            if(this->client_list.size() == 0)
                std::cout<<"The server is empty!\n";
            else{
                std::cout<<"The server is not empty\n";
                for(std::vector<Client*>::iterator it = this->client_list.begin();it!=this->client_list.end();it++)
                {
                    std::cout<<"Client name: "<<(*it)->id<<'\n';
                    std::cout<<"Client pid: "<<(*it)->pid<<'\n';
                }
            }
        }
    }
    //Create a specific thread for listening new client
    
    //Create list of thread correspond to each room in the server
}

void Server::registClient(std::string id, int roomNumb,pid_t pid)
{
    Client* client = new Client(id,roomNumb,pid);
    this->client_list.push_back(client);
    //Check if room exist
    for(std::vector<Room*>::iterator it = this->room_list.begin();it!=this->room_list.end();it++)
    {
        //If room is exist then add new client
        if((*it)->numb == roomNumb)
        {
            (*it)->addClient(client);
            return;
        }
    }

    //If not create new room
    Room* room = new Room(roomNumb);
    this->room_list.push_back(room);
    //Then add new client
    std::cout<<"adding new client\n";
    room->addClient(client);
}

int main()
{
    server = new Server();
    struct sigaction siga;
    sigaction(SIGINT,&siga,NULL);
    server->run();
}