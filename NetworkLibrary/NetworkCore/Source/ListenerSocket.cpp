#include "ListenerSocket.h"
#include <iostream>
#include <fcntl.h>

ListenerSocket::ListenerSocket(uint16_t port, int backlog)
    : mListenSocket{-1}, mPort{port}, mBacklog{backlog}
{
}
ListenerSocket::~ListenerSocket()
{
    Close();
}

ListenerSocket::ListenerSocket(ListenerSocket &&other) noexcept
    : mListenSocket(other.mListenSocket), mPort(other.mPort), mBacklog(other.mBacklog)
{
    other.mListenSocket = -1;
}
ListenerSocket &ListenerSocket::operator=(ListenerSocket &&other) noexcept
{
    if (this != &other)
    {
        Close();
        mListenSocket = other.mListenSocket;
        mPort = other.mPort;
        mBacklog = other.mBacklog;

        other.mListenSocket = -1;
    }

    return *this;
}

eListenerSocketError ListenerSocket::Open()
{
    Close();

    mListenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (mListenSocket < 0)
    {
        std::perror("[ListenerSocket::Open] socket creation failed");
        return ListenerSocket_OpenFailed;
    }
    std::printf("[ListenerSocket::Open] socket fd=%d\n", mListenSocket);

    int flags = ::fcntl(mListenSocket, F_GETFL, 0);
    if (flags >= 0)
    {
        ::fcntl(mListenSocket, F_SETFL, flags | O_NONBLOCK);
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = ::htonl(INADDR_ANY);
    address.sin_port = ::htons(mPort);

    if (::bind(mListenSocket, reinterpret_cast<sockaddr *>(&address), static_cast<socklen_t>(sizeof(address))) < 0)
    {
        Close();
        std::perror("[ListenerSocket::Open] bind failed");
        return ListenerSocket_BindFailed;
    }
    std::printf("[ListenerSocket::Open] bound to port %u\n", static_cast<unsigned>(mPort));

    if (::listen(mListenSocket, mBacklog) < 0)
    {
        Close();
        std::perror("[ListenerSocket::Open] listen failed");
        return ListenerSocket_ListenFailed;
    }
    std::printf("[ListenerSocket::Open] listening with backlog %d\n", mBacklog);

    return ListenerSocket_Ok;
}
eListenerSocketError ListenerSocket::Accept(Socket &outServerSocket)
{
    if (mListenSocket < 0)
        return ListenerSocket_InvalidState;

    sockaddr_in clientAddress{};
    socklen_t clientAddressLen = static_cast<socklen_t>(sizeof(clientAddress));

    int clientSocket = ::accept(mListenSocket, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLen);
    if (clientSocket < 0)
    {
        std::perror("[ListenerSocket::Accept] accept failed");
        return ListenerSocket_AcceptFailed;
    }

    outServerSocket.Close();
    outServerSocket = Socket(clientSocket);

    std::printf("[ListenerSocket::Open] listening on port %u\n", static_cast<unsigned>(mPort));
    return ListenerSocket_Ok;
}

bool ListenerSocket::IsOpen() const
{
    return mListenSocket >= 0;
}

void ListenerSocket::Close()
{
    if (mListenSocket >= 0)
    {
        ::close(mListenSocket);
        mListenSocket = -1;
    }
}

int ListenerSocket::GetFd() const
{
    return mListenSocket;
}