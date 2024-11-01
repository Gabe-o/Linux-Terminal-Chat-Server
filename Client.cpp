#include "thread.h"
#include "socket.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

using namespace Sync;

// This thread handles the connection to the server
class ClientThread : public Thread
{
private:
	// Reference to our connected socket
	Socket &socket;

	// Reference to boolean flag for terminating the thread
	bool &terminate;

	// Are we connected?
	bool connected = false;

	// Data to send to server
	ByteArray data;
	std::string data_str;
	int expectedLength = 0;

public:
	ClientThread(Socket &socket, bool &terminate)
		: socket(socket), terminate(terminate)
	{
	}

	~ClientThread()
	{
	}

	void TryConnect()
	{
		try
		{
			std::cout << "Connecting...";
			std::cout.flush();
			int result = socket.Open();
			connected = true;
			std::cout << "OK" << std::endl;
		}
		catch (std::string exception)
		{
			std::cout << "FAIL (" << exception << ")" << std::endl;
			return;
		}
	}

	virtual long ThreadMain()
	{
		// Initially we need a connection
		while (true)
		{
			// Attempt to connect
			TryConnect();

			// Check if we are exiting or connection was established
			if (terminate || connected)
			{
				break;
			}

			// Try again every 5 seconds
			std::cout << "Trying again in 5 seconds" << std::endl;
			sleep(5);
		}

		while (!terminate)
		{
			// We are connected, perform our operations
			std::cout << "Please input your data (done to exit): ";
			std::cout.flush();

			// Get the data
			data_str.clear();
			std::getline(std::cin, data_str);
			data = ByteArray(data_str);
			expectedLength = data_str.size();

			// Must have data to send
			if (expectedLength == 0)
			{
				std::cout << "Cannot send no data!" << std::endl;
				continue;
			}
			else if (data_str == "done")
			{
				std::cout << "Closing the client..." << std::endl;
				terminate = true;
				break;
			}

			// Write to the server
			socket.Write(data);

			// Get the response
			socket.Read(data);
			data_str = data.ToString();

			// If the length is different, the socket must be closed
			if (data_str.size() != expectedLength)
			{
				std::cout << "Server failed to respond. Closing client..." << std::endl;
				terminate = true;
			}
			else
			{
				std::cout << "Server Response: " << data_str << std::endl;
			}

			// Are we terminating
			if (terminate)
			{
				break;
			}
		}

		return 0;
	}
};

int main(void)
{
	// Welcome the user and try to initialize the socket
	std::cout << "SE3313 Lab 4 Client" << std::endl;

	// Create our socket
	// Socket socket("127.0.0.1", 3000);
	// Socket socket("35.162.177.130", 3000);
	Socket socket("172.30.37.177", 3000);
	bool terminate = false;

	// Scope to kill thread
	{
		// Thread to perform socket operations on
		ClientThread clientThread(socket, terminate);

		while (!terminate)
		{
			// This will wait for 'done' to shutdown the server
			sleep(1);
		}

		// Wait to make sure the thread is cleaned up
	}
	socket.Close();

	return 0;
}
