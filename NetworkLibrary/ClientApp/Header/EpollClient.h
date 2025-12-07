#pragma once

#include <sys/epoll.h>
#include <memory>
#include "Session.h"
#include "Socket.h"

class EpollClient
{
public:
    EpollClient(const char* serverIp, uint16_t serverPort,
                size_t recvBufSize, size_t sendBufSize);
    ~EpollClient();

    bool Start();    
    void Run();  
    void Stop();

    eSessionError Send(const void* data, size_t len);

private:
    void HandleEvent(uint32_t events);
    bool CheckConnectCompleted(uint32_t events);

private:
    const char* mServerIp;
    uint16_t    mServerPort;
    size_t      mRecvBufSize;
    size_t      mSendBufSize;

    int mEpollFd;
    bool mRunning;

    std::unique_ptr<Session> mSession;
    bool mConnecting;
};
