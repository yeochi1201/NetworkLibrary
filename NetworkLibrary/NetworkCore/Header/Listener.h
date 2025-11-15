#ifndef LISTENER_H
#define LISTENER_H

#include <cstdint>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

typedef enum eListenerError {
	Listener_Ok = 0,
	Listener_SocketFailed,
	Listener_BindFailed,
	Listener_ListenFailed,
	Listener_AcceptFailed,
	Listener_SendFailed,
	Listener_RecvFailed
};

class Listener {
public:
	explicit Listener(uint16_t port, int backlog = 5);
	~Listener();

	Listener(const Listener&)			= delete;
	Listener& operator=(const Listener) = delete;

	Listener(Listener&& other)				noexcept;
	Listener& operator=(Listener&& other)	noexcept;

	eListenerError	Open();
	eListenerError	AcceptClient();
	eListenerError	SendAll(const void* data, std::size_t length);
	eListenerError	RecvSome(void* buffer, std::size_t maxLength);
	
	bool			IsOpen() const;
	bool			HasClient() const;

	void			CloseClient();

private:
	int				mListenSocket;
	int				mClientSocket;
	std::uint16_t	mPort;
	int				mBacklog;
		
};

#endif