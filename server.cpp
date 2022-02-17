/*Author: Tran Manh Dung*/

#include "type.h"

struct Client{
    //Id of client
    std::string id;
    //List of rooms this client's in
    unsigned int roomNum;
    pid_t pid;
    Client(){
        this->id = "";
        this->pid = -1;
    }
    Client(std::string id, int roomNumb, pid_t pid):id{id},pid{pid},roomNum{roomNumb}{};
    friend class Server;
    friend class Room;
};

class Room{
    //Room id
    unsigned int numb;
    std::vector<Client*> clientList;
    //Shared memory pointer
    Message* shmptr;
    key_t key;
    int ShmID;
    //Add new client
    void addClient(Client*);    
    public:
        void removeClient(pid_t);
        Room():numb{0},shmptr{nullptr}{};
        Room(unsigned int numb);
        unsigned int getNumb(){return this->numb;}
        ~Room();
        void broadcast(pid_t sender);
    friend class Server;
};

Room::Room(unsigned int numb):numb{numb},shmptr{NULL}{
    this->key = ftok(".",numb);
    this->ShmID = shmget(this->key,sizeof(Message),(IPC_CREAT)|0666);
    if(this->ShmID==-1)
    {
        std::cout<<"Error, try again!"<<'\n';
    }
    this->shmptr = (Message*)shmat(this->ShmID,NULL,0);
}

Room::~Room()
{
    shmdt(shmptr);
    for(auto it = this->clientList.begin();it!=this->clientList.end();it++)
        delete (*it);
}

void Room::removeClient(pid_t pid)
{
    for(std::vector<Client*>::iterator it = this->clientList.begin();it!=this->clientList.end();it++)
        if((*it)->pid == pid)
            this->clientList.erase(it--);
}

void Room::addClient(Client* client){
    this->clientList.push_back(client);
    kill(client->pid,SIGUSR2);
    std::string mess = "< "+ client->id+" has joined the chat >";
    for(int i = 0; i < mess.size();i++)
        this->shmptr->mess[i]=mess[i];
    strcpy(this->shmptr->name,"Server");
    broadcast(client->pid);
}

void Room::broadcast(pid_t sender)
{
    for(std::vector<Client*>::iterator it = this->clientList.begin();it!=this->clientList.end();it++)
        if((*it)->pid!=sender)
            kill((*it)->pid,SIGUSR2);
}

//Using Singleton design pattern
class Server{
    bool running;
    pid_t pid;  
    key_t key;
    int ShmId;
    //List of rooms the server has
    public:
    std::vector<Room*> room_list;
    //List of people are in rooms of the server
    std::vector<Client*> client_list;
    //Room run
    Server();
    static Server server;
    public:
        static Server& get()
        { 
            return server; 
        }
        ~Server();
        Serverinfo *ShmPtr;
        void registClient(std::string id, int roomNumb,pid_t pid);
        void stop();
        void printClient();
        void printRoom();   
        void createRoom();
        void deleteRoom();
};

Server Server::server;

Server::Server()
{
    this->running=false;
    this->pid=getpid();
    Serverinfo serinfo;
    serinfo.pid=getpid();
    this->key=ftok(".",'s');
    this->ShmId=shmget(key,sizeof(Serverinfo),IPC_CREAT|0666);
    this->ShmPtr=(Serverinfo*)shmat(ShmId,NULL,0);
    *(this->ShmPtr)=serinfo;
}

Server::~Server()
{
    shmdt(this->ShmPtr);
    for(auto it = this->room_list.begin();it!=room_list.end();it++)
        delete (*it);
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
    Room* newRoom = new Room(roomNumb);
    this->room_list.push_back(newRoom);
    //Then add new client
    newRoom->addClient(client);
    //Signal to client that he is now in the chat room
}

void Server::printClient(){
    int i = 0;
    if(this->client_list.size() == 0)
    {
        std::cout<<"There is no client in the server now\n";
        return;
    }
    for(std::vector<Client*>::iterator it = this->client_list.begin();it!=this->client_list.end();it++)
    {
        std::cout<<"Client number "<<i<<"\t";
        std::cout<<"Name: "<<(*it)->id<<'\t';
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

static void getInfo(int sig, const siginfo_t siginfo, void* context){
    if(sig==SIGUSR1)
    {
        pid_t sender_pid=siginfo.si_pid;
        std::cout<<"Someone wants to joint!, with pid: "<<sender_pid<<'\n';
        kill(sender_pid,SIGUSR1);
        Info info;
        key_t key = ftok(".",sender_pid);
        int ShmID = shmget(key, sizeof(Info), 0666); 
        void* ShmPtr = (void*)shmat(ShmID,NULL,0);
        while((int*)ShmPtr==(int*)-1){
            ShmID=shmget(key,sizeof(Info),0666);
            ShmPtr = (void*)shmat(ShmID,NULL,0);
        }
        while(!((Info*)ShmPtr)->complete){}
        Server::get().registClient(((Info*)ShmPtr)->name,((Info*)ShmPtr)->room,((Info*)ShmPtr)->pid);
        shmdt(ShmPtr);
    }
}

static void broadcast(pid_t sender)
{
    int roomNum = 0;
    for(std::vector<Client*>::iterator it = Server::get().client_list.begin();it!=Server::get().client_list.end();it++)
    {
        if((*it)->pid == sender)
            roomNum=(*it)->roomNum;   
    }
    for(std::vector<Room*>::iterator it = Server::get().room_list.begin();it!=Server::get().room_list.end();it++)
        if((*it)->getNumb() == roomNum)
            (*it)->broadcast(sender);
}

static void handleSignal(int sig, siginfo_t* siginfo, void* context)
{
    if(sig==SIGUSR1)
    {
        std::cout<<siginfo->si_pid<<'\n';
        std::thread my_thread(getInfo,sig,*siginfo,context);   
        my_thread.detach();
    }
    else if(sig==SIGUSR2)
    {
        std::thread my_thread(broadcast,siginfo->si_pid);
        my_thread.detach();
    }
    else if(sig==SIGHUP)
    {
        for(std::vector<Client*>::iterator it = Server::get().client_list.begin();it!=Server::get().client_list.end();it++)
        {
            if((*it)->pid == siginfo->si_pid)
            {
                unsigned int roomNumb = (*it)->roomNum;
                for(std::vector<Room*>::iterator it = Server::get().room_list.begin();it!=Server::get().room_list.end();it++)
                    if((*it)->getNumb()==roomNumb)
                    {
                        (*it)->removeClient(siginfo->si_pid);
                    }
                delete (*it);
                Server::get().client_list.erase(it--);
            }
        }
    }
    else if(sig==SIGINT)
    {
        exit(0);
    }
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
            Server::get().printClient();
        else if(a=='r')
            Server::get().printRoom();
    }
}

static void listener()
{
    struct sigaction siga1;
    siga1.sa_sigaction=handleSignal;
    siga1.sa_flags=SA_SIGINFO;
    sigaction(SIGUSR1,&siga1,NULL);
    sigaction(SIGUSR2,&siga1,NULL);
    sigaction(SIGHUP,&siga1,NULL);
    sigaction(SIGINT,&siga1,NULL);
    while(true){}
}

int main()
{
    std::thread print_thread(print);
    print_thread.detach();
    std::thread listen(listener);
    listen.detach();
    while(true){
        if(Server::get().ShmPtr->pid!=getpid())
            Server::get().ShmPtr->pid=getpid();
        sleep(1);
    }
}