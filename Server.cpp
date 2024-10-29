#include "thread.h"
#include "socketserver.h"
#include "Blockable.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <vector>
#include <cstring>
#include <algorithm>

using namespace Sync;

class ClientSocketThread : public Thread
{
    private:
        Socket& socket;
        std::vector<ClientSocketThread*>& cSockets;
        ThreadSem cSocketsSem;

    public:
        bool done = false;

        ClientSocketThread(Socket& socket, std::vector<ClientSocketThread*>& cSockets, ThreadSem cSocketsSem) 
        : socket(socket), cSockets(cSockets), cSocketsSem(cSocketsSem) {
            cSocketsSem.Wait();
            cSockets.push_back(this);
            cSocketsSem.Signal();
        }

        ~ClientSocketThread() {
            cSocketsSem.Wait();
            cSockets.erase(std::remove(cSockets.begin(),cSockets.end(), this), cSockets.end());
            cSocketsSem.Signal();
            delete &socket;
            delete this;
        }

        Socket& GetSocket() {
            return socket;
        }

        virtual long ThreadMain() {
            try {
                while(!done) {
                    // Reading client's message
                    ByteArray byteMsg;
                    int status = socket.Read(byteMsg);
                    // Checking if an error has occured
                    if(status < 0) {
                        std::cout << "Error status " << status << ", is less than 0." << std::endl;
                        break;
                    }
                    else if(status==0) {
                        std::cout << "Socket was closed by client." << std::endl;
                        break;
                    }

                    // Transforming client's message to uppercase and appending !
                    std::string msg = byteMsg.ToString();
                    for (char& c : msg) {
                        c = std::toupper(c);
                    }
                    msg.append("!");

                    // Sending Message back to client
                    byteMsg = ByteArray(msg);
                    status = socket.Write(byteMsg);
                    // Checking if an error has occured
                    if(status != msg.size()) {
                        std::cout << "Error writing message to client. Socket was likely closed by client." << std::endl;
                        break;
                    }
                }
            }
            catch(std::string e) {
                std::cout << e << std::endl;
            }

            return 0;
        }
};


class ServerThread : public Thread
{
    private:
        SocketServer& server;
        std::vector<ClientSocketThread*> cSockets;
        ThreadSem cSocketsSem;
    public:
        bool done = false;

        ServerThread(SocketServer& server) : server(server), cSocketsSem(1) {}

        ~ServerThread() {
            // Closing all clients
            cSocketsSem.Wait();
            for (ClientSocketThread* socketThread : cSockets) {
                socketThread->done = true;
                Socket& socket = socketThread->GetSocket();
                socket.Close();
            }
            cSocketsSem.Signal();
        }

        virtual long ThreadMain() {
            while(!done) {
                try {
                    // Creating socket for client connection
                    Socket socket = server.Accept();
                    Socket* socketP = new Socket(socket);
                    Socket& socketRef = *socketP;

                    // Creating Client Socket Thread
                    new ClientSocketThread(socketRef, cSockets, cSocketsSem);
                }
                catch(std::string& e) {
                    if(!done) {
                        std::cout << e << std::endl;
                    }
                }
                catch(TerminationException& e) {
                    std::cout << e << std::endl;
                }
            }

            std::cout << "Closed Server Thread" << std::endl;
            return 0;
        }
};

int main(void)
{
    std::cout << "I am a server." << std::endl;

    try{
        // Starting server
        SocketServer server(5000);
        ServerThread serverThread(server);

        // Asking for stop command
        while(true) {
            std::string in;
            std::getline(std::cin, in);

            if(in=="stop") break;
        }

        // Stopping server
        serverThread.done = true;
        server.Shutdown();
    }
    catch(std::string& e) {
        std::cout << e << std::endl;
    }

    return 0;
}
