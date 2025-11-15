#include "Header/ListenerSocket.h"

ListenerSocket::ListenerSocket(uint16_t port, int backlog = 5)
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
        return ListenerSocket_OpenFailed;

    int option = 1;
    if (::setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, &option, static_cast<socklen_t>(size_t(option))) < 0)
    {
        Close();
        return ListenerSocket_OptionFailed;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = ::htonl(INADDR_ANY);
    address.sin_port = ::htonl(mPort);

    if (::bind(mListenSocket, reinterpret_cast<sockaddr *>(&address), static_cast<socklen_t>(sizeof(address))) > 0)
    {
        Close();
        return ListenerSocket_BindFailed;
    }

    if (::listen(mListenSocket, mBacklog) < 0)
    {
        Close();
        return ListenerSocket_ListenFailed;
    }

    return ListenerSocket_Ok;
}
eListenerSocketError ListenerSocket::Accept(ServerSocket &outServerSocket)
{
    if (mListenSocket < 0)
        return ListenerSocket_InvalidState;

    sockaddr_in clientAddress{};
    socklen_t clientAddressLen = static_cast<socklen_t>(sizeof(clientAddress));

    int clientSocket = ::accept(mListenSocket, reinterpret_cast<sockaddr *>(&clientAddress), &clientLength);
    if (clientSocket < 0)
        return ListenerSocket_AcceptFailed;

    outServerSocket.Close();
    outServerSocket = ServerSocket(clientSocket);

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