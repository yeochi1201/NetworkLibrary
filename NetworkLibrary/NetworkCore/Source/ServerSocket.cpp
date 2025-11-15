#include "Header/ServerSocket.h"

ServerSocket::ServerSocket()
    : mSocketFd{-1}
{
}

explicit ServerSocket::ServerSocket(int socketFd)
    : mSocketFd{socketFd}
{
}

ServerSocket::~ServerSocket()
{
    Close();
}

ServerSocket::ServerSocket(ServerSocket &&other) noexcept
    : mSocketFd(other.mSocketFd)
{
    other.mSocketFd = -1;
}

ServerSocket &ServerSocket::operator=(ServerSocket &&other) noexcept
{
    if (this != &other)
    {
        Close();
        mSocketFd = other.mSocketFd;
        other.mSocketFd = -1;
    }

    return *this;
}

bool ServerSocket::IsOpen() const
{
    return mSocketFd >= 0;
}

eServerSocketError ServerSocket::Send(const void *data, std::size_t length, std::size_t &outSent)
{
    outSent = 0;
    if (!IsOpen())
        return ServerSocket_InvalidState;

    const char *bytes = static_cast<const char *>(data);
    std::size_t remain = length;

    while (remain > 0)
    {
        size_t sent = ::send(mSocketFd, bytes + outSent, remain, 0);
        if (sent <= 0)
            return ServerSocket_SendFailed;

        remain -= static_cast<std::size_t>(sent);
        bytes += sent;
        outSent += static_cast<std::size_t>(sent);
    }
    return ServerSocket_Ok;
}

eServerSocketError ServerSocket::Recv(void *buffer, std::size_t maxLength, std::size_t &outReceived)
{
    outReceived = 0;

    if (!IsOpen())
    {
        return ServerSocket_InvalidState;
    }

    ssize_t received = ::recv(mSocketFd, buffer, maxLength, 0);
    if (received < 0)
    {
        return ServerSocket_RecvFailed;
    }

    outReceived = static_cast<std::size_t>(received);
    return ServerSocket_Ok;
}

void ServerSocket::Close()
{
    if (mSocketFd >= 0)
    {
        ::close(mSocketFd);
        mSocketFd = -1;
    }
}