#ifndef SESSION_H
#define SESSION_H
#include "Socket.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"

#include <functional>
#include <chrono>

enum eSessionError{
    Session_Ok = 0,
    Session_NotOpen,
    Session_AlreadyOpen,
    Session_InvalidArgs,
    Session_SocketError,
    Session_RecvBufferError,
    Session_SendBufferError,
    Session_InternalError,
    Session_PeerClosed
};

enum eSessionState{
    SessionState_Closed = 0,
    SessionState_Opening,
    SessionState_Open,
    SessionState_Closing
};


class Session{
public:
    using RecvCallback = std::function<void(Session&, RecvBuffer&)>;
    using SendCallback = std::function<void(Session&, size_t)>;
    using CloseCallback =  std::function<void(Session&)>;
public:
    Session(size_t recvBufSize, size_t sendBufSize, int socketFd);
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    Session(Session&& other) noexcept;
    Session& operator=(Session&& other) noexcept;

    eSessionError Open(size_t recvBufSize, size_t sendBufSize);
    void Close();

    bool IsOpen() const;

    eSessionError PollRecv();
    eSessionError FlushSend();
    eSessionError QueueSend(const void* data, size_t len);

    void SetRecvCallback(RecvCallback callback);
    void SetSendCallback(SendCallback callback);
    void SetCloseCallback(CloseCallback callback);

    int Fd() const;
    eSessionState State() const;

    RecvBuffer& RecvBuf() noexcept;
    const RecvBuffer& RecvBuf() const noexcept;

    SendBuffer& SendBuf() noexcept;
    const SendBuffer& SendBuf() const noexcept;

    std::chrono::steady_clock::time_point& LastActiveTime() noexcept;

private:
    void InvokeRecvCallback();
    void InvokeSendCallback(size_t sentBytes);
    void InvokeCloseCallback();
private:
    Socket      mSocket;
    RecvBuffer  mRecvBuffer;
    SendBuffer  mSendBuffer;

    eSessionState mState;
    RecvCallback  mRecvCallback;
    SendCallback  mSendCallback;
    CloseCallback mCloseCallback;

    std::chrono::steady_clock::time_point mLastActive;
};

#endif