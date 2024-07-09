#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <iomanip>
#include <algorithm> 

#include "AES.h"

#pragma comment(lib, "Ws2_32.lib")

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::thread;
using std::condition_variable;
using std::mutex;
using std::atomic;
using std::setfill;
using std::lock_guard;
using std::unique_lock;
using std::stringstream;
using std::hex;
using std::setw;
using std::fill;
using std::begin;
using std::end;



AES aes;
string key;
string iv;

void clearLine() {
	// Move cursor up one line
	cout << "\033[1A";
	// Clear the entire line
	cout << "\033[2K";
	// Move cursor to beginning of the line
	cout << "\r";
	cout.flush();
}


string EncryptMessage(string& message) {
	vector<unsigned char> plainText(message.begin(), message.end());
	vector<unsigned char> KEY(key.begin(), key.end());
	vector<unsigned char> IV(iv.begin(), iv.end());
	vector<unsigned char> tmp = aes.EncryptCBC(plainText, KEY, IV);

	// Convert the encrypted data to a hexadecimal string
	stringstream ss;
	for (size_t i = 0; i < tmp.size(); i++)
	{
		ss << hex << setw(2) << setfill('0') << static_cast<int>(tmp[i]);
	}
	string hexCiphertext = ss.str();

	return hexCiphertext;
}

string DecryptMessage(string& encryptedMessage) {
	// Convert the hexadecimal ciphertext to a byte array
	vector<unsigned char> ciphertext(encryptedMessage.length() / 2);
	vector<unsigned char> KEY(key.begin(), key.end());
	vector<unsigned char> IV(iv.begin(), iv.end());
	for (size_t i = 0; i < ciphertext.size(); i++)
	{
		string byteStr = encryptedMessage.substr(i * 2, 2);
		ciphertext[i] = static_cast<unsigned char>(stoi(byteStr, nullptr, 16));
	}
	vector<unsigned char> decryptedMessage = aes.DecryptCBC(ciphertext, KEY, IV);

	return string(decryptedMessage.begin(), decryptedMessage.end());
}


bool Initialize() {
	WSADATA data;

	return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

void SendMsg(SOCKET s, atomic<bool>& usernameSet, mutex& mtx, condition_variable& cv) {

	cout << "Enter your username: " ;
	string name;
	getline(cin, name);
	{
		lock_guard<mutex> lock(mtx);
		usernameSet.store(true);
	}
	cv.notify_one();

	string message;
	while (1) {
		cout << "Your message: ";
		getline(cin, message);
		clearLine();

		cout << "[" << name << "] : " << message<<endl;
		string encrypt = EncryptMessage(message);
		string msg = name + " : " + encrypt + '\n';
		int bytesSend = send(s, msg.c_str(), msg.length(), 0);
		if (bytesSend == SOCKET_ERROR) {
			cerr << "Error Sending message" << endl;
			break;
		}

		if (message == "quit") {
			cout << "Stopping the application" << endl;
			break;
		}
	}
	closesocket(s);
	WSACleanup();
}

void ReceiveMsg(SOCKET s, atomic<bool>& usernameSet, mutex& mtx, condition_variable& cv) {
	{
		unique_lock<mutex> lock(mtx);
		cv.wait(lock, [&usernameSet] { return usernameSet.load(); });
	}

	char buffer[4096];
	int receiveLength;
	while (1) {
		receiveLength = recv(s, buffer, sizeof(buffer), 0);
		if (receiveLength <= 0) {
			cerr << "Disconnected from the server" << endl;
			break;
		}
		else {
			string message(buffer, receiveLength);

			int pos = message.find(" : ");
			string name = message.substr(0, pos);
			string encryptedMessage = message.substr(pos + 3, receiveLength-1);

			string decryptedMessage = DecryptMessage(encryptedMessage);
			cout << endl << "[" << name << "] : " << decryptedMessage << endl;
		}
	}
	closesocket(s);
	WSACleanup();
}

int main() {
	
	if (!Initialize())
	{
		cerr << "Winsock failed" << endl;
	}

	SOCKET s;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {
		cerr << "Invalid socket created" << endl;
		return 1;
	}

	int port = 8080;
	string serveraddress = "127.0.0.1"; //this is localhost(add any public host ip as preffered)
	sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	inet_pton(AF_INET, serveraddress.c_str(), &(serveraddr.sin_addr));

	//connect to the server
	if (connect(s, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR) {
		cerr << "Not able to connect to the server" << ": " << WSAGetLastError();
		closesocket(s);
		WSACleanup();
		return 1;
	}

	cout << "Successfully connected to the server" << endl;

	// Receive the key from the server
	char keyBuffer[256];
	int bytesReceived = recv(s, keyBuffer, sizeof(keyBuffer), 0);
	if (bytesReceived <= 0) {
		cout << "Error receiving key from server" << endl;
		closesocket(s);
		WSACleanup();
		return 1;
	}

	string tmp(keyBuffer, bytesReceived);
	key = tmp;

	// Receive the iv from the server
	char keyBuffer1[256];
	int bytesReceived1 = recv(s, keyBuffer1, sizeof(keyBuffer1), 0);
	if (bytesReceived1 <= 0) {
		cout << "Error receiving key from server" << endl;
		closesocket(s);
		WSACleanup();
		return 1;
	}
	tmp=string(keyBuffer1, bytesReceived1);
	iv = tmp;


	atomic<bool> usernameSet(false);
	mutex mtx;
	condition_variable cv;

	//send/recieve
	thread senderThread(SendMsg, s, ref(usernameSet), ref(mtx), ref(cv));
	thread receiverThread(ReceiveMsg, s, ref(usernameSet), ref(mtx), ref(cv));

	senderThread.join();
	receiverThread.join();

	return 0;
}