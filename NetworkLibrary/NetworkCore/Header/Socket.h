#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

#include <cstdint>
#include <cstddef>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

enum eSocketError
{
    Socket_Ok = 0,
    Socket_InvalidState,
    Socket_SendFailed,
    Socket_RecvFailed,
    Socket_WouldBlock
};

class Socket
{
public:
    Socket();
    explicit Socket(int socketFd);
    ~Socket();

    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;

    Socket(Socket &&other) noexcept;
    Socket &operator=(Socket &&other) noexcept;

    bool IsOpen() const;
    int GetFd() const;

    eSocketError Send(const void *data, std::size_t length, std::size_t &outSent);
    eSocketError Recv(void *buffer, std::size_t maxLength, std::size_t &outReceived);

    eSocketError SetBlocking(bool blocking);

    void Close();

private:
    int mSocketFd;
    friend class ListenerSocket;
};

#endif