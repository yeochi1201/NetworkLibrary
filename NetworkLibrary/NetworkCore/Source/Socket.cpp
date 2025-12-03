#include "Socket.h"
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

Socket::Socket()
    : mSocketFd{-1}
{
}

Socket::Socket(int socketFd)
    : mSocketFd{socketFd}
{
}

Socket::~Socket()
{
    Close();
}

Socket::Socket(Socket &&other) noexcept
    : mSocketFd(other.mSocketFd)
{
    other.mSocketFd = -1;
}

Socket &Socket::operator=(Socket &&other) noexcept
{
    if (this != &other)
    {
        Close();
        mSocketFd = other.mSocketFd;
        other.mSocketFd = -1;
    }

    return *this;
}

bool Socket::IsOpen() const
{
    return mSocketFd >= 0;
}
int Socket::GetFd() const
{
    return mSocketFd;
}

eSocketError Socket::Send(const void *data, std::size_t length, std::size_t &outSent)
{
    outSent = 0;
    if (!IsOpen())
        return Socket_InvalidState;

    const char *bytes = static_cast<const char *>(data);
    std::size_t remain = length;

    while (remain > 0)
    {
        ssize_t sent = ::send(mSocketFd, bytes + outSent, remain, 0);
        if (sent <= 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return Socket_WouldBlock;
            }
            return Socket_SendFailed;
        }
        if (sent == 0)
        {
            return Socket_SendFailed;
        }

        bytes += sent;
        remain -= static_cast<std::size_t>(sent);
        outSent += static_cast<std::size_t>(sent);
    }
    return Socket_Ok;
}

eSocketError Socket::Recv(void *buffer, std::size_t maxLength, std::size_t &outReceived)
{
    outReceived = 0;

    if (!IsOpen())
    {
        return Socket_InvalidState;
    }

    ssize_t received = ::recv(mSocketFd, buffer, maxLength, 0);
    if (received < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return Socket_WouldBlock;
        }
        return Socket_RecvFailed;
    }

    outReceived = static_cast<std::size_t>(received);
    return Socket_Ok;
}

eSocketError Socket::SetBlocking(bool blocking)
{
    if (!IsOpen())
    {
        return Socket_InvalidState;
    }

    int flags = ::fcntl(mSocketFd, F_GETFL, 0);
    if (flags < 0)
    {
        return Socket_RecvFailed;
    }

    if (blocking)
    {
        flags &= ~O_NONBLOCK;
    }
    else
    {
        flags |= O_NONBLOCK;
    }

    if (::fcntl(mSocketFd, F_SETFL, flags) < 0)
    {
        return Socket_RecvFailed;
    }

    return Socket_Ok;
}

void Socket::Close()
{
    if (mSocketFd >= 0)
    {
        ::close(mSocketFd);
        mSocketFd = -1;
    }
}