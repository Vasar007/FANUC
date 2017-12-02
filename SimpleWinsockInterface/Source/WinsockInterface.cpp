#include <iostream>
#include <cassert>
#include <thread>
#include <string>

#include "Utilities.h"
#include "WinsockInterface.h"


namespace vasily
{
	
const std::unordered_map<int, std::string> WinsockInterface::_TABLE_OF_ERRORS = 
{
	{ 6,		"Specified event object handle is invalid." },
	{ 8,		"Insufficient memory available." },
	{ 87,		"One or more parameters are invalid." },
	{ 995,		"Overlapped operation aborted." },
	{ 996,		"Overlapped I/O event object not in signaled state." },
	{ 997,		"Overlapped operations will complete later." },
	{ 10004,	"Interrupted function call." },
	{ 10009,	"File handle is not valid." },
	{ 10013,	"Permission denied." },
	{ 10014,	"Bad address" },
	{ 10022,	"Invalid argument." },
	{ 10024,	"Too many open files." },
	{ 10035,	"Resource temporarily unavailable." },
	{ 10036,	"Operation now in progress." },
	{ 10037,	"Operation already in progress." },
	{ 10038,	"Socket operation on nonsocket." },
	{ 10039,	"Destination address required." },
	{ 10040,	"Message too long." },
	{ 10041,	"Protocol wrong type for socket." },
	{ 10042,	"Bad protocol option." },
	{ 10043,	"Protocol not supported." },
	{ 10044,	"Socket type not supported." },
	{ 10045,	"Operation not supported." },
	{ 10046,	"Protocol family not supported." },
	{ 10047,	"Address family not supported by protocol family." },
	{ 10048,	"Address already in use." },
	{ 10049,	"Cannot assign requested address." },
	{ 10050,	"Network is down." },
	{ 10051,	"Network is unreachable." },
	{ 10052,	"Network dropped connection on reset." },
	{ 10053,	"Software caused connection abort." },
	{ 10054,	"Connection reset by peer." },
	{ 10055,	"No buffer space available." },
	{ 10056,	"Socket is already connected." },
	{ 10057,	"Socket is not connected." },
	{ 10058,	"Cannot send after socket shutdown." },
	{ 10059,	"Too many references." },
	{ 10060,	"Connection timed out." },
	{ 10061,	"Connection refused." },
	{ 10062,	"Cannot translate name." },
	{ 10063,	"Name too long." },
	{ 10064,	"Host is down." },
	{ 10065,	"No route to host." },
	{ 10066,	"Directory not empty." },
	{ 10067,	"Too many processes." },
	{ 10068,	"User quota exceeded." },
	{ 10069,	"Disk quota exceeded." },
	{ 10070,	"Stale file handle reference." },
	{ 10071,	"Item is remote." },
	{ 10091,	"Network subsystem is unavailable." },
	{ 10092,	"Winsock.dll version out of range." },
	{ 10093,	"Successful WSAStartup not yet performed." },
	{ 10101,	"Graceful shutdown in progress." },
	{ 10102,	"No more results." },
	{ 99999,	"Some another error occured..." }
};

WinsockInterface::WinsockInterface()
	: _wsaData()
	, _socketSend()
	, _socketReceive()
	, _socketSendAddress()
	, _socketReceiveAddress()
	, _addressInfo(nullptr)
	, _isRunning(false)
	, _isInitialized(false)
	, _buffer(new char[_MAXRECV])
	, _message(new char[_MAXRECV])
{
}

WinsockInterface::~WinsockInterface() noexcept
{
	// Delete buffers.
	delete[] _buffer;
	delete[] _message;

	if (_isInitialized)
	{
		// The closing of the socket and Winsock.
		closesocket(_socketSend);
		closesocket(_socketReceive);
		WSACleanup();
	}
}

void WinsockInterface::init()
{
	_isInitialized = true;

	initWinsock(_wsaData);
	initSocket(_socketSend);
	initSocket(_socketReceive);
}

void WinsockInterface::initWinsock(WSADATA& wsaData) const
{
	// Initialization Winsock implementation.
	utils::print("Initializing Winsock...");

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
	{
		utils::println("\n\nFAILED. ERROR CODE:", WSAGetLastError());
		std::cin.get();
		exit(static_cast<int>(ErrorType::FAILED_INITIALIZE_WSDATA));
	}

	utils::println(" done.");
}

void WinsockInterface::initSocket(SOCKET& socketToInit, const int aiProtocol) const
{
	// Initialization socket.
	utils::println("Initialized socket.");

	socketToInit = socket(AF_INET, SOCK_STREAM, aiProtocol);

	if (socketToInit == INVALID_SOCKET)
	{
		utils::println("\nCOULD NOT CREATE SOCKET.");
		std::cin.get();
		exit(static_cast<int>(ErrorType::FAILED_CREATE_SOCKET));
	}

	utils::println("Socket created.");

}

void WinsockInterface::bindSocket(const SOCKET& socketToBind, SOCKADDR_IN& socketAddress, 
									const int port) const
{
	u_short usPort = static_cast<u_short>(port);

	// Set socket settings.
	socketAddress.sin_family			= AF_INET;
	socketAddress.sin_addr.S_un.S_addr	= INADDR_ANY;
	socketAddress.sin_port				= htons(usPort);

	// Binding to a specific address and port.
	if (bind(socketToBind, reinterpret_cast<SOCKADDR*>(&socketAddress),
		sizeof socketAddress) == SOCKET_ERROR)
	{
		utils::println("\nBIND FAILED.");
		std::cin.get();
		exit(static_cast<int>(ErrorType::FAILED_BIND));
	}

	utils::println("Bind done.");
}

void WinsockInterface::listenOn(const SOCKET& socketToList, const int backlog) const
{
	// Check if input data is correct (backlog should only positive value).
	assert(backlog > 0);

	// Include "listening" mode for receiving incoming connections.
	if (listen(socketToList, backlog) == SOCKET_ERROR)
	{
		utils::println("\nLISTEN FAILED.");
		std::cin.get();
		exit(static_cast<int>(ErrorType::FAILED_LISTEN));
	}

	utils::println("Enabled listening.");

	// Cleaning addresses.
	freeaddrinfo(_addressInfo.get());
}

bool WinsockInterface::tryConnect(const int port, const std::string& ip, 
									const SOCKET& socketToConnect, SOCKADDR_IN& socketAddress) const
{
	const char* serverIP	= ip.c_str();
	const u_short usPort	= static_cast<u_short>(port);

	// Set socket settings.
	socketAddress.sin_family	= AF_INET;
	socketAddress.sin_port		= htons(usPort);
	inet_pton(AF_INET, serverIP, &socketAddress.sin_addr);

	// The connection to the server.
	if (connect(socketToConnect, reinterpret_cast<SOCKADDR*>(&socketAddress),
		sizeof socketAddress) == SOCKET_ERROR)
	{
		utils::println("\nCONNECTION TO SERVER WAS FAILED.");

		return false;
	}

	utils::println("Connected successfully.\n");
	
	return true;
}

bool WinsockInterface::isRun() const
{
	return _isRunning;
}

void WinsockInterface::sendData(const SOCKET& socketToSend, const std::string& data) const
{
	const char* dataChar = data.c_str();

	// Sending data on socket.
	if (send(socketToSend, dataChar, strlen(dataChar), 0) == SOCKET_ERROR)
	{
		utils::println("SEND FAILED.");
		return;
	}

	utils::println("Sent data:", data, "successfully.\n");
}

void WinsockInterface::setTimeout(const SOCKET& socketForSetting, const long seconds,
									const long microseconds) const
{
	TIMEVAL timeout;
	timeout.tv_sec = seconds;
	timeout.tv_usec = microseconds;
	setsockopt(socketForSetting, SOL_SOCKET, SO_RCVTIMEO,
				reinterpret_cast<char*>(&timeout), sizeof timeout);
}

}
