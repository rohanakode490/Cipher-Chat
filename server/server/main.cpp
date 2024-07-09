#include <iostream>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <tchar.h>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>

#include "Threadpool.h"

#pragma comment(lib, "ws2_32.lib")

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::stringstream;
using std::hex;
using std::setw;
using std::setfill;
using std::runtime_error;

string key;
string iv;

string generate_random_hex_string(size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::stringstream ss;
    for (size_t i = 0; i < length / 2; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return ss.str();
}

// initialize socket
bool Initialize()
{
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

void InteractWithClient(SOCKET clientSocket, vector<SOCKET>& clients) {

    // Sending key to client
    int bytesSent = send(clientSocket, key.c_str(), key.length(), 0);
    if (bytesSent == SOCKET_ERROR) {
        cerr << "Error sending key to client: " << WSAGetLastError() << endl;
        closesocket(clientSocket);
        return;
    }

    int bytesSent1 = send(clientSocket, iv.c_str(), iv.length(), 0);
    if (bytesSent1 == SOCKET_ERROR) {
        cerr << "Error sending iv to client: " << WSAGetLastError() << endl;
        closesocket(clientSocket);
        return;
    }
    cout << "Sent key" << endl;

    char buffer[4096];
    while (1) {
        fd_set readfds;
        struct timeval timeout;

        // Initialize the fd_set structures
        FD_ZERO(&readfds);

        // Add the client socket to the readfds set
        FD_SET(clientSocket, &readfds);

        // Set the timeout value
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100 milliseconds

        // Wait for the socket to become ready
        int ready = select(0, &readfds, nullptr, nullptr, &timeout);
        if (ready == SOCKET_ERROR) {
            cerr << "Select error: " << WSAGetLastError() << endl;
            break;
        }

        // Check if the client socket is ready for reading
        if (FD_ISSET(clientSocket, &readfds)) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) {
                cerr << "Client Disconnected" << endl;
                break;
            }
            
            string message(buffer, bytesReceived);
            auto it = message.find(':');
            string name = message.substr(0, it-1);
            cout << "message from client : " << name << endl;
            

            for (auto client : clients) {
                if (client != clientSocket) // sending to every client except self
                {
                  send(client, message.c_str(), message.length(), 0);
                }
            }
        }
    }

    // Remove the client after it is disconnected
    auto it = find(clients.begin(), clients.end(), clientSocket);
    if (it != clients.end()) {
        clients.erase(it);
    }

    closesocket(clientSocket);
}


int main()
{
    // Setting key values for encryption on client
    key = generate_random_hex_string(200);
    iv= generate_random_hex_string(200);

    if (!Initialize())
    {
        cerr << "Winsock failed" << endl;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0); // TCP type of protocol

    if (listenSocket == INVALID_SOCKET)
    {
        cerr << "socket creation failed" << endl;
        return 1;
    }

    u_long iMode = 1;
    ioctlsocket(listenSocket, FIONBIO, &iMode);

    // creating address structure
    int port = 8080;
    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);

    // convert the ipaddress(0.0.0.0) put inside the sin_family in binary format
    if (InetPton(AF_INET, _T("0.0.0.0"), &serveraddr.sin_addr) != 1)
    {
        cerr << "Setting address Structure failed" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // bind
    if (::bind(listenSocket, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR)
    {
        cerr << "Binding failed" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // listen
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) // parameter => listenSocket, number of clients it can have in the queue(SOMAXCONN = maximum number)
    {
        cerr << "Listen Failed" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server has started listening on port: " << port << endl;
    
    vector<SOCKET> clients;
    Threadpool pool(16); 
    fd_set readfds;
    struct timeval timeout;
    while (1) {
        // Initialize the fd_set structures
        FD_ZERO(&readfds);

        // Add the server socket to the readfds set
        FD_SET(listenSocket, &readfds);

        // Set the timeout value
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100 milliseconds

        // Wait for sockets to become ready
        int ready = select(0, &readfds, nullptr, nullptr, &timeout);
        if (ready == SOCKET_ERROR) {
            cerr << "Select error: " << WSAGetLastError() << endl;
            continue;
        }

        // Check if the server socket is ready for reading (new connection)
        if (FD_ISSET(listenSocket, &readfds)) {
            SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) {
                cerr << "Invalid Client Socket" << endl;
                continue;
            }

            // Add the new client socket to the list of client sockets
            clients.push_back(clientSocket);
            pool.ExecuteTask(InteractWithClient, clientSocket, ref(clients));
        }
    }

    closesocket(listenSocket);
    WSACleanup();

    return 0;
}