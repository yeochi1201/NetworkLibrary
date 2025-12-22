#include "EpollServer.h"
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>

EpollServer::EpollServer(uint16_t port, size_t recvBufSize, size_t sendBufSize)
    : mEpollFd(-1), mRunning(false), mListener(port, 100), mRecvBufSize(recvBufSize), mSendBufSize(sendBufSize)
{
}

EpollServer::~EpollServer()
{
    Stop();
    if (mEpollFd != -1)
    {
        close(mEpollFd);
        mEpollFd = -1;
    }
}

bool EpollServer::Start(){
 if (mListener.Open() != ListenerSocket_Ok)
    {
        std::cerr << "Listener open failed\n";
        return false;
    }

    mEpollFd = ::epoll_create1(0);
    if (mEpollFd < 0)
    {
        std::perror("epoll_create1");
        return false;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = mListener.GetFd();

    if (::epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mListener.GetFd(), &ev) < 0)
    {
        std::perror("epoll_ctl ADD listener");
        return false;
    }

    mRunning = true;
    return true;   
}
void EpollServer::UpdateWriteInterest(int fd, bool enable){
    epoll_event ev{};
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    if(enable) ev.events |= EPOLLOUT;

    ::epoll_ctl(mEpollFd, EPOLL_CTL_MOD, fd, &ev);
}

void EpollServer::HandleNewConnection(){
    for(;;){
        Socket clientSocket;
        eListenerSocketError acceptErr = mListener.Accept(clientSocket);
        if (acceptErr == ListenerSocket_AcceptFailed)
        {
            break;
        }
        else if (acceptErr != ListenerSocket_Ok)
        {
            std::cerr << "Accept failed\n";
            break;
        }

        clientSocket.SetBlocking(false);
        auto session = std::make_unique<Session>(mRecvBufSize, mSendBufSize, std::move(clientSocket));
        if(session->Open(mRecvBufSize, mSendBufSize) != Session_Ok){
            continue;
        }

        int fd = session->Fd();
        session->SetRecvCallback([](Session& s, RecvBuffer& rb){
            //TODO Handle RecvCallback
            std::uint8_t buf[4096];

            while (!rb.IsEmpty())
            {
                std::size_t readBytes = 0;
                eRecvBufferError err = rb.Read(buf, sizeof(buf), readBytes);
                if (err != RecvBuf_Ok || readBytes == 0)
                    break;

                eSessionError se = s.QueueSend(buf, readBytes);
                if (se != Session_Ok)
                {
                    break;
                }
            }
        });
        session->SetCloseCallback([this](Session& s){
            int fd = s.Fd();
            ::epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, nullptr);
            mSessions.erase(fd);
        });
        session->SetFrameCallback([] (Session& s, const std::uint8_t* p, std::size_t n){
            s.SendFrame(p, n);
        });
        session->SetWriteInterestCallback([this](Session& s, bool enable){
            this->UpdateWriteInterest(s.Fd(), enable);
        });
        
        
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
        ev.data.fd = session->Fd();

        if (::epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &ev) < 0)
        {
            std::perror("epoll_ctl ADD client");
            continue;
        }

        mSessions.emplace(fd, std::move(session));
    }
}

void EpollServer::HandleClientEvent(int fd, uint32_t events){
    auto it= mSessions.find(fd);
    if (it == mSessions.end())
        return;

    Session& sess = *(it->second);

    if (events & (EPOLLERR | EPOLLHUP))
    {
        sess.Close();
        return;
    }

    if (events & EPOLLIN)
    {
        eSessionError r = sess.OnReadable();
        if (r == Session_PeerClosed || r == Session_SocketError || r == Session_RecvBufferError)
        {
            return;
        }
    }

    if (events & EPOLLOUT)
    {
        eSessionError w = sess.OnWritable();
        if (w == Session_SocketError)
        {
            return;
        }
    }
}

void EpollServer::Run()
{
    constexpr int MAX_EVENTS = 64;
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
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == mListener.GetFd())
            {
                HandleNewConnection();
            }
            else
            {
                HandleClientEvent(fd, ev);
            }
        }
    }
}

void EpollServer::Stop()
{
    mRunning = false;
    if (mEpollFd >= 0)
    {
        ::close(mEpollFd);
        mEpollFd = -1;
    }
    mListener.Close();
    mSessions.clear();
}
