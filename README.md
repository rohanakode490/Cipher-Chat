# Cipher-Chat - Multi-Client TCP Server with Thread Pool

## Project Overview

This project implements a multi-threaded TCP server in C++ using the Winsock API. The server supports multiple client connections concurrently using a thread pool to manage tasks. Each client communicates with the server and receives a key and initialization vector (IV) for encryption purposes. Messages sent by one client are broadcast to all other connected clients.

### Key Features:
- **Thread Pooling:** The server uses a custom thread pool to handle multiple client connections concurrently, optimizing resource management.
- **Non-blocking I/O:** The server socket is non-blocking, using `select()` to wait for activity.
- **Message Broadcasting:** The server broadcasts any received message from one client to all other connected clients.
- **Dynamic Key Generation:** On each connection, the server generates and sends unique encryption keys
- **AES Encryption:** The client includes AES encryption functionality (defined in AES.h and AES.cpp), ready for future use to secure communication.

## Folder Structure
```.
├── client
│   ├── src
│   │   ├── main.cpp          # Client-side source code
│   │   ├── AES.h             # AES encryption header
│   │   ├── AES.cpp           # AES encryption implementation
│   ├── X64
│   │   └── Debug
│   │       └── client.exe    # Compiled client executable
├── server
│   ├── src
│   │   ├── main.cpp          # Server-side source code
│   │   ├── Threadpool.h      # Threadpool header for handling multiple connections
│   ├── X64
│   │   └── Debug
│   │       └── server.exe    # Compiled server executable
└── README.md                 # Project documentation
```

### Files Description

- **main.cpp**: 
  - Initializes the server.
  - Generates random encryption keys and IVs to be sent to the clients.
  - Manages incoming client connections and messages.
  - Uses a thread pool to handle each client connection without blocking the main thread.
  - Implements the main loop for receiving messages and broadcasting them to other clients.

- **Threadpool.h**: 
  - Implements a thread pool that allows for the efficient execution of tasks (in this case, handling client connections).
  - Uses a queue to manage tasks and assigns them to available threads.
  - Threads are reused for handling new clients and perform tasks concurrently.

## How the Code Works

### Key and IV Generation
- The server generates a random **key** and **IV** using the `generate_random_hex_string` function. These values are sent to clients for future encryption purposes.
  
### Client Interaction
- Each client connects to the server through a TCP socket.
- The server sends the key and IV to each client upon connection.
- Clients can send messages to the server, which are then broadcast to all other connected clients (except the sender).

### Thread Pool
- The `Threadpool` class is responsible for managing a fixed number of worker threads.
- Tasks (such as handling individual client sockets) are added to a queue and processed by the next available thread.

## Concepts Applied
- **Socket Programming**: TCP-based server-client communication using Winsock.
- **Multithreading**: Efficient client handling using a thread pool to avoid creating/destroying threads repeatedly.
- **Concurrency Control**: Use of `mutex` and `condition_variable` to safely manage shared resources in the thread pool.
- **Non-blocking I/O**: Using `select()` to handle multiple sockets without blocking the main thread.
- **Random Key Generation**: Generates random hex strings for encryption keys and IVs.


## How to Run

You can run the project using either **Visual Studio 2022** or by executing the precompiled binaries located in the `X64/Debug` directories for both the server and client.

### Option 1: Using Visual Studio 2022

#### Requirements:
- **Visual Studio 2022** (with C++ development environment setup)
- Windows operating system (the project relies on Winsock2 for socket communication)

1. **Open the Project**: 
   Open the solution files for both the server and client in Visual Studio 2022.

2. **Build the Project**: 
   Build both the server and client by selecting `Build` > `Build Solution` in Visual Studio.

3. **Run the Server**:
   - Start the server by pressing `F5` or `Ctrl+F5` from the Visual Studio environment.
   - The server listens for client connections on port `8080`.

4. **Run the Client**:
   - Once the server is running, open another instance of Visual Studio and run the client in the same way (`F5` or `Ctrl+F5`).
   - The client will connect to the server and you can begin exchanging messages.

### Option 2: Using Precompiled Executables

If you do not want to use Visual Studio 2022, you can directly run the compiled executables available in the `X64/Debug` folders for both the client and server.

#### Running the Server:
1. Navigate to the `server/X64/Debug` folder.
2. Run the `server.exe` file.

The server will start listening for client connections on port `8080`.

#### Running the Client:
1. Navigate to the `client/X64/Debug` folder.
2. Run the `client.exe` file.
The client will attempt to connect to the server at `localhost` (127.0.0.1) on port `8080`.

## Project Workflow

### Server
- The server listens for incoming client connections.
- Once a client connects, the server:
- Sends encryption keys (`key` and `iv` values).
- Uses the thread pool to handle communication between clients.
- Receives and broadcasts messages to all other connected clients.

### Client
- The client establishes a connection to the server.
- It receives encryption keys from the server.
- The client can send messages to the server, which will then broadcast them to all other clients.

- **AES Encryption**: The client has AES encryption methods implemented in `AES.h` and `AES.cpp`. While encryption is currently not applied to transmitted messages, these files lay the foundation for securing communication in the future.


## Dependencies
- **Winsock2**: The project uses the Winsock2 library for networking on Windows. Ensure your environment supports Winsock2 for proper functionality.

To include the necessary Winsock library in Visual Studio, the following directive is used:
```cpp
#pragma comment(lib, "ws2_32.lib")

---
```
## Future Improvement
- Add GUI support for both client and server using libraries like Qt or WinForms.
- Support for more advanced features like file sharing, error handling, and more robust encryption.
