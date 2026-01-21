#ifndef LISTENER_SOCKET_H
#define LISTENER_SOCKET_H

#include <cstdint>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Socket.h"

enum eListenerSocketError
{
	ListenerSocket_Ok = 0,
	ListenerSocket_OpenFailed,
	ListenerSocket_OptionFailed,
	ListenerSocket_BindFailed,
	ListenerSocket_ListenFailed,
	ListenerSocket_AcceptFailed,
	ListenerSocket_InvalidState,
	ListenerSocket_WouldBlock
};

class ListenerSocket
{
public:
	explicit ListenerSocket(uint16_t port, int backlog = 5);
	~ListenerSocket();

	ListenerSocket(const ListenerSocket &) = delete;
	ListenerSocket &operator=(const ListenerSocket) = delete;

	ListenerSocket(ListenerSocket &&other) noexcept;
	ListenerSocket &operator=(ListenerSocket &&other) noexcept;

	eListenerSocketError Open();
	eListenerSocketError Accept(Socket &outServerSocket);

	bool IsOpen() const;

	void Close();

	int GetFd() const;

private:
	int mListenSocket;
	uint16_t mPort;
	int mBacklog;
};

#endif