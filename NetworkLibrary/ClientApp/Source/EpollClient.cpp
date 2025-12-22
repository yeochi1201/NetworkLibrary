// EpollClient.cpp
#include "EpollClient.h"

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <iostream>



static uint32_t BuildClientEvents(bool connecting, bool wantSendOut)
{
    uint32_t ev = EPOLLIN | EPOLLERR | EPOLLHUP;
    if (connecting || wantSendOut) ev |= EPOLLOUT;
    return ev;
}


EpollClient::EpollClient(const char* serverIp, uint16_t serverPort,
                         size_t recvBufSize, size_t sendBufSize)
    : mServerIp(serverIp)
    , mServerPort(serverPort)
    , mRecvBufSize(recvBufSize)
    , mSendBufSize(sendBufSize)
    , mEpollFd(-1)
    , mConnecting(false)
    , mWantSendOut(false)
    , mRunning(false)
{
}

EpollClient::~EpollClient()
{
    Stop();
    if (mEpollFd != -1)
    {
        ::close(mEpollFd);
        mEpollFd = -1;
    }
}

bool EpollClient::Start()
{
    if (mRunning) return true;

    // 1) epoll 생성
    mEpollFd = ::epoll_create1(0);
    if (mEpollFd < 0)
    {
        std::perror("epoll_create1");
        return false;
    }

    // 2) non-blocking connect
    Socket sock;
    const eSocketError cErr = sock.Connect(mServerIp, mServerPort, /*nonBlocking=*/true);
    if (cErr == Socket_Ok)
    {
        mConnecting = false;
    }
    else if (cErr == Socket_ConnectInProgress)
    {
        mConnecting = true;
    }
    else
    {
        std::cerr << "connect failed\n";
        return false;
    }

    // 3) Session 생성 & Open
    mSession = std::make_unique<Session>(mRecvBufSize, mSendBufSize, std::move(sock));
    if (mSession->Open(mRecvBufSize, mSendBufSize) != Session_Ok)
    {
        std::cerr << "session open failed\n";
        mSession.reset();
        return false;
    }

    // 4) Session -> EpollClient : send 목적 EPOLLOUT 토글 요청 콜백
    //    CONNECTING 상태면 EPOLLOUT은 강제로 유지됨(아래 BuildClientEvents 로직)
    mSession->SetWriteInterestCallback([this](Session& /*s*/, bool enable) {
        mWantSendOut = enable;

        epoll_event ev{};
        ev.data.fd = mSession->Fd();
        ev.events  = BuildClientEvents(mConnecting, mWantSendOut);

        if (::epoll_ctl(mEpollFd, EPOLL_CTL_MOD, mSession->Fd(), &ev) < 0)
        {
            std::perror("epoll_ctl MOD (write interest)");
        }
    });

    // 5) epoll 등록 (CONNECTING이면 EPOLLOUT 포함)
    epoll_event ev{};
    ev.data.fd = mSession->Fd();
    ev.events  = BuildClientEvents(mConnecting, mWantSendOut);

    if (::epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mSession->Fd(), &ev) < 0)
    {
        std::perror("epoll_ctl ADD client");
        mSession.reset();
        return false;
    }

    mRunning = true;
    return true;
}

void EpollClient::Run()
{
    epoll_event events[kMaxEvents];

    while (mRunning)
    {
        int n = ::epoll_wait(mEpollFd, events, kMaxEvents, -1);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            std::perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i)
        {
            HandleEvent(events[i].events);
            if (!mRunning) break;
        }
    }
}

void EpollClient::Stop()
{
    mRunning = false;

    if (mSession)
    {
        const int fd = mSession->Fd();
        if (mEpollFd >= 0)
        {
            ::epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, nullptr);
        }

        mSession->Close();
        mSession.reset();
    }
}

eSessionError EpollClient::Send(const void* data, size_t len)
{
    if (!mSession || !mSession->IsOpen())
        return Session_NotOpen;

    // framing 사용 중이면 아래를 SendFrame으로 바꿔도 됨:
    // return mSession->SendFrame(data, len);
    return mSession->QueueSend(data, len);
}

void EpollClient::HandleEvent(uint32_t events)
{
    if (!mSession) return;

    // 1) 에러/끊김 우선 처리
    if (events & (EPOLLERR | EPOLLHUP))
    {
        mSession->Close();
        mRunning = false;
        return;
    }

    // 2) EPOLLOUT: connecting이면 connect 완료 확인 먼저, 아니면 flush
    if (events & EPOLLOUT)
    {
        if (mConnecting)
        {
            // 완료면 내부에서 mConnecting=false + interest MOD까지 수행
            (void)CheckConnectCompleted(events);
        }
        else
        {
            const eSessionError w = mSession->OnWritable();
            if (w == Session_SocketError || w == Session_SendBufferError)
            {
                mRunning = false;
                return;
            }
            // OnWritable()에서 버퍼 empty 되면 Session이 disable 요청 -> MOD 발생
        }
    }

    // 3) EPOLLIN
    if (events & EPOLLIN)
    {
        const eSessionError r = mSession->OnReadable();
        if (r == Session_PeerClosed || r == Session_SocketError || r == Session_RecvBufferError)
        {
            mRunning = false;
            return;
        }
        // OnReadable() 중 응답으로 QueueSend() 호출되면 enable 요청 -> MOD 발생
    }
}

bool EpollClient::CheckConnectCompleted(uint32_t /*events*/)
{
    if (!mSession) return false;

    int so_error = 0;
    socklen_t slen = sizeof(so_error);

    if (::getsockopt(mSession->Fd(), SOL_SOCKET, SO_ERROR, &so_error, &slen) < 0)
    {
        std::perror("getsockopt(SO_ERROR)");
        mSession->Close();
        mRunning = false;
        return false;
    }

    if (so_error != 0)
    {
        std::cerr << "connect failed: " << ::strerror(so_error) << "\n";
        mSession->Close();
        mRunning = false;
        return false;
    }

    mConnecting = false;

    // connect 직후 send buffer가 이미 차있으면 EPOLLOUT 필요
    mWantSendOut = mSession->HasPendingSend();

    // CONNECTING 종료 후: wantSendOut에 따라 EPOLLOUT 제거 가능
    epoll_event ev{};
    ev.data.fd = mSession->Fd();
    ev.events  = BuildClientEvents(mConnecting, mWantSendOut);

    if (::epoll_ctl(mEpollFd, EPOLL_CTL_MOD, mSession->Fd(), &ev) < 0)
    {
        std::perror("epoll_ctl MOD after connect");
        mSession->Close();
        mRunning = false;
        return false;
    }

    return true;
}
