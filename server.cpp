#include<string>
#include<vector>
class Client{
    //Id of client
    std::string id;
    //List of rooms this client's in
    std::vector<unsigned int> roomList;
};

class Room{
    //Number of clients
    int clientNumber;
    //Shared memory pointer
    void* shmptr;
    //Update shared memory
    void set();
    //Get message from one of the clients
    void get();
    //Broadcast to all clients
    void broadcast();    
};
//Using Singleton design pattern
class Server{
    //List of rooms the server has
    std::vector<int> room_list;
    //List of people are in rooms of the server
    std::vector<Client> client_list;
    //Client register
    void registClient(std::string id, int roomNumb);
    //Client unregist
    void unregistClient(Client& client);
    //Room create
    void createRoom();
    //Room delete
    void deleteRoom();
    public:
        void run();
};

int main()
{
    Server* server = new Server();
    server->run();
}