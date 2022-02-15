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


struct Message{
    char mess[200];
};

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
    //Room id
    public:
    unsigned int numb;
    std::vector<Client*> clientList;
    //Shared memory pointer
    Message* shmptr;
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
        Room(unsigned int numb):numb{numb},shmptr{nullptr}{
            key_t MyKey = ftok(".",numb);
            std::cout<<"Common key room: "<<MyKey<<'\n';
            int ShmID = shmget(MyKey,sizeof(Message),IPC_CREAT|0666);
            shmptr = (Message*)shmat(ShmID,NULL,0);
        };
    friend class Server;
};

Room* room;

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
    public:
    std::vector<Room*> room_list;
    //List of people are in rooms of the server
    std::vector<Client*> client_list;
    //Client register
    //Client unregist
    void unregistClient(Client& client);
    //Client listner
    //Room create
    void createRoom();
    //Room delete
    void deleteRoom();
    //Room run
    void runRoom(Room&);
    public:
        Server();
        ~Server();
        void registClient(std::string id, int roomNumb,pid_t pid);
        void stop();
        void printClient();
        void printRoom();
};

Server* server;

static void listenClient()
{
        
}
static void getInfo(int sig, const siginfo_t siginfo, void* context){
    if(sig==SIGUSR1)
    {
        pid_t sender_pid=siginfo.si_pid;
        std::cout<<"Someone wants to joint!, with pid: "<<sender_pid<<'\n';
        kill(sender_pid,SIGUSR1);
        Info info;
        key_t key = ftok(".",sender_pid);
        std::cout<<"Common key: "<<key<<'\n';
        int ShmID = shmget(key, sizeof(Info), 0666); 
        void* ShmPtr = (void*)shmat(ShmID,NULL,0);
        while((int*)ShmPtr==(int*)-1){
            //std::cout<<"Waiting for you...\n";
            ShmID=shmget(key,sizeof(Info),0666);
            ShmPtr = (void*)shmat(ShmID,NULL,0);
        }
        std::cout<<"Oke waiting it to be complete!\n";
        while(!((Info*)ShmPtr)->complete){}
        std::cout<<"Client name: "<<((Info*)ShmPtr)->name<<'\n';
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
    if(sig==SIGUSR1)
    {
        std::thread my_thread(getInfo,sig,*siginfo,context);   
        my_thread.detach();
    }
    else if(sig==SIGUSR2)
    {
        std::cout<<"Received message! "<<room->shmptr->mess<<'\n';
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
}

Server::~Server()
{
    std::cout<<"Get out\n";
}

void Server::stop(){
    this->running=false;
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
    room = new Room(roomNumb);
    this->room_list.push_back(room);
    //Then add new client
    room->addClient(client);
    //Signal to client that he is now in the chat room
    kill(client->pid,SIGUSR2);
}

void Server::printClient(){
    int i = 0;
    for(std::vector<Client*>::iterator it = this->client_list.begin();it!=this->client_list.end();it++)
    {
        std::cout<<"Client number "<<i<<":\n";
        std::cout<<"Name: "<<(*it)->id<<'\n';
        std::cout<<"Pid: "<<(*it)->pid<<'\n';
        i++;    
    }
}

void Server::printRoom(){
    std::cout<<"List of room: "<<'\n';
    for(std::vector<Room*>::iterator it = this->room_list.begin();it!=this->room_list.end();it++)
    {
        std::cout<<"room: "<<(*it)->numb<<'\t';
    }
    std::cout<<'\n';
}

//This function print information that we want to know
static void print()
{
    while(true){
        char a = 'x';
        std::cout<<"Enter c to show list of clients in the server\n";
        std::cout<<"Enter r to show list of rooms in the server\n";
        std::cin>>a;
        //sleep(1);
        if(a=='c')
            server->printClient();
        else if(a=='r')
            server->printRoom();
    }
}

int main()
{
    std::thread print_thread(print);
    print_thread.detach();
    server = new Server();
    struct sigaction siga1;
    siga1.sa_sigaction=newComeHandler;
    siga1.sa_flags=SA_SIGINFO;
    sigaction(SIGUSR1,&siga1,NULL);
    sigaction(SIGUSR2,&siga1,NULL);
    while(true){
    }
}