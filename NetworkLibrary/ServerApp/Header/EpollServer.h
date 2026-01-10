#pragma once

#include <unordered_map>
#include <sys/epoll.h>
#include "ListenerSocket.h"
#include "Session.h"
#include "HttpParser.h"
class EpollServer
{
public:
    struct HttpConnState{
        HttpParser parser;
        bool closeAfterSend = false;
    };

    EpollServer(uint16_t port, size_t recvBufSize, size_t sendBufSize);
    ~EpollServer();

    bool Start();
    void Run();
    void Stop();

    void UpdateWriteInterest(int fd, bool enable);
private:
    void HandleNewConnection();
    void HandleClientEvent(int fd, uint32_t events);

    int mEpollFd;
    bool mRunning;
    ListenerSocket mListener;
    size_t mRecvBufSize;
    size_t mSendBufSize;

    std::unordered_map<int, std::unique_ptr<Session>> mSessions;
    std::unordered_map<int, HttpConnState> mHttpStates;
};