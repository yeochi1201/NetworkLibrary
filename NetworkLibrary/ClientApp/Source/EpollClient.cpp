#include "EpollClient.h"
#include <unistd.h>
#include <iostream>

EpollClient::EpollClient(const char* serverIp, uint16_t serverPort,
                         size_t recvBufSize, size_t sendBufSize)
    : mServerIp(serverIp)
    , mServerPort(serverPort)
    , mRecvBufSize(recvBufSize)
    , mSendBufSize(sendBufSize)
    , mEpollFd(-1)
    , mRunning(false)
    , mConnecting(false)
{
}

EpollClient::~EpollClient()
{
    Stop();
}

bool EpollClient::Start(){
    Socket sock;
    eSocketError err = sock.Connect(mServerIp, mServerPort, true);
    if(err == Socket_ConnectFailed){
        std::cerr << "Connect failed\n";
        return false;
    }

    mSession = std::make_unique<Session>(mRecvBufSize, mSendBufSize, std::move(sock));
    eSessionError sErr = mSession->Open(mRecvBufSize, mSendBufSize);
    if(sErr != Session_Ok){
        std::cerr << "Session open failed\n";
        mSession.reset();
        return false;
    }

    mEpollFd = epoll_create1(0);
    if(mEpollFd < 0){
        std::cerr << "Epoll create failed\n";
        return false;
    }

    epoll_event ev{};
    ev.data.fd = mSession->Fd();
    if(err == Socket_ConnectInProgress){
        ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
        mConnecting = true;
    } else {
        ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
        mConnecting = false;
    }

    if(::epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mSession->Fd(), &ev) < 0){
        std::cerr << "Epoll ctl add failed\n";
        return false;
    }

    mSession->SetRecvCallback([](Session& session, RecvBuffer& recvBuf){
        std::cout << "Received data\n";
    });

    mSession->SetCloseCallback([this](Session& session){
        std::cout << "Session closed by server\n";
        Stop();
    });

    mRunning = true;
    return true;
}

bool EpollClient::CheckConnectCompleted(uint32_t events)
{
    if (!mConnecting)
        return true;

    if (events & (EPOLLERR | EPOLLHUP))
    {
        std::cerr << "connect failed (EPOLLERR/HUP)\n";
        mRunning = false;
        return false;
    }

    if (events & EPOLLOUT)
    {
        int err = 0;
        socklen_t len = sizeof(err);
        if (::getsockopt(mSession->Fd(), SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0)
        {
            std::cerr << "connect failed, so_error=" << err << "\n";
            mRunning = false;
            return false;
        }

        std::cout << "connect completed\n";
        mConnecting = false;

        epoll_event ev{};
        ev.data.fd = mSession->Fd();
        ev.events  = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
        ::epoll_ctl(mEpollFd, EPOLL_CTL_MOD, mSession->Fd(), &ev);

        return true;
    }

    return true;
}

void EpollClient::HandleEvent(uint32_t events)
{
    if (!mSession || !mSession->IsOpen())
        return;

    if (!CheckConnectCompleted(events))
        return;

    if (events & (EPOLLERR | EPOLLHUP))
    {
        mSession->Close();
        return;
    }

    if (events & EPOLLIN)
    {
        mSession->OnReadable();
    }

    if (!mConnecting && (events & EPOLLOUT))
    {
        mSession->OnWritable();
    }
}

void EpollClient::Run()
{
    constexpr int MAX_EVENTS = 8;
    epoll_event events[MAX_EVENTS];

    while (mRunning)
    {
        int n = ::epoll_wait(mEpollFd, events, MAX_EVENTS, -1);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            std::perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i)
        {
            HandleEvent(events[i].events);
        }
    }
}

void EpollClient::Stop()
{
    mRunning = false;
    if (mEpollFd >= 0)
    {
        ::close(mEpollFd);
        mEpollFd = -1;
    }
    if (mSession)
    {
        mSession->Close();
        mSession.reset();
    }
}

eSessionError EpollClient::Send(const void* data, size_t len)
{
    if (!mSession || !mSession->IsOpen())
        return Session_NotOpen;

    return mSession->SendFrame(data, len);
}
