#include "Header/Listener.h"

Listener::Listener(uint16_t port, int backlog = 5)
    : mListenSocket{-1}, mClientSocket{-1}, mPort{port}, mBacklog{backlog}
{
}
Listener::~Listener()
{
    CloseClient();
    CloseListener();
}

Listener::Listener(Listener &&other) noexcept
    : mListenSocket(other.mListenSocket), mClientSocket(other.mClientSocket), mPort(other.mPort), mBacklog(other.mBacklog)
{
    other.mListenSocket = -1;
    other.mClientSocket = -1;
}
Listener &Listener::operator=(Listener &&other) noexcept
{
    if (this != &other)
    {
        CloseClient();
        CloseListener();
        mListenSocket = other.mListenSocket;
        mClientSocket = other.mClientSocket;
        mPort = other.mPort;
        mBacklog = other.mBacklog;

        other.mListenSocket = -1;
        other.mClientSocket = -1;
    }

    return *this;
}

eListenerError Listener::Open()
{
    CloseClient();
    CloseListener();

    mListenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (mListenSocket < 0)
        return Listener_SocketOpenFailed;

    int option = 1;
    if (::setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, &option, static_cast<socklen_t>(size_t(option))) < 0)
    {
        CloseListener();
        return Listner_SocketOptionFailed;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = ::htonl(INADDR_ANY);
    address.sin_port = ::htonl(mPort);

    if (::bind(mListenSocket, reinterpret_cast<sockaddr *>(&address), static_cast<socklen_t>(sizeof(address))) > 0)
    {
        CloseListener();
        return Listener_BindFailed;
    }

    if (::listen(mListenSocket, mBacklog) < 0)
    {
        CloseListener();
        return Listener_ListenFailed;
    }

    return Listener_Ok;
}
eListenerError Listener::AcceptClient()
{
}
eListenerError Listener::SendAll(const void *data, std::size_t length)
{
}
eListenerError Listener::RecvSome(void *buffer, std::size_t maxLength)
{
}

bool Listener::IsOpen() const
{
}
bool Listener::HasClient() const
{
}

void CloseListner()
{
}
void Listener::CloseClient()
{
}