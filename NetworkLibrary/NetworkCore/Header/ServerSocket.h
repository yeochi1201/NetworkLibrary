#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

#include <cstdint>
#include <cstddef>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

typedef enum eServerSocketError
{
    ServerSocket_Ok = 0,
    ServerSocket_InvalidState,
    ServerSocket_SendFailed,
    ServerSocket_RecvFailed
};

class ServerSocket
{
public:
    ServerSocket();
    explicit ServerSocket(int socketFd);
    ~ServerSocket();

    ServerSocket(const ServerSocket &) = delete;
    ServerSocket &operator=(const ServerSocket &) = delete;

    ServerSocket(ServerSocket &&other) noexcept;
    ServerSocket &operator=(ServerSocket &&other) noexcept;

    bool IsOpen() const;

    eServerSocketError Send(const void *data, std::size_t length, std::size_t &outSent);
    eServerSocketError Recv(void *buffer, std::size_t maxLength, std::size_t &outReceived);

    void Close();

private:
    int mSocketFd;

    friend class ListenerSocket;
};

#endif