#include "Session.h"

Session::Session(size_t recvBufSize, size_t sendBufSize, int socketFd)
    : mSocket(socketFd), mRecvBuffer(recvBufSize), mSendBuffer(sendBufSize), mState(SessionState_Closed), mLastActive(std::chrono::steady_clock::now())
{
}

Session::~Session()
{
    Close();
}

Session::Session(Session &&other) noexcept
    : mSocket(std::move(other.mSocket)), mRecvBuffer(std::move(other.mRecvBuffer)), mSendBuffer(std::move(other.mSendBuffer)), mState(other.mState), mRecvCallback(std::move(other.mRecvCallback)), mSendCallback(std::move(other.mSendCallback)), mCloseCallback(std::move(other.mCloseCallback)), mLastActive(other.mLastActive)
{
    other.mState = SessionState_Closed;
    other.mSendCallback = nullptr;
    other.mRecvCallback = nullptr;
    other.mCloseCallback = nullptr;
}

Session &Session::operator=(Session &&other) noexcept
{
    if (this != &other)
    {
        mSocket = std::move(other.mSocket);
        mRecvBuffer = std::move(other.mRecvBuffer);
        mSendBuffer = std::move(other.mSendBuffer);
        mState = other.mState;
        mRecvCallback = std::move(other.mRecvCallback);
        mSendCallback = std::move(other.mSendCallback);
        mCloseCallback = std::move(other.mCloseCallback);
        mLastActive = other.mLastActive;

        other.mState = SessionState_Closed;
        other.mSendCallback = nullptr;
        other.mRecvCallback = nullptr;
        other.mCloseCallback = nullptr;
    }
    return *this;
}

eSessionError Session::Open(size_t recvBufSize, size_t sendBufSize)
{
    mState = SessionState_Opening;

    if (mState == SessionState_Open || mState == SessionState_Opening)
        return Session_AlreadyOpen;

    mRecvBuffer = RecvBuffer(recvBufSize);
    mSendBuffer = SendBuffer(sendBufSize);

    eSendBufferError sErr = mSendBuffer.Open();
    if (sErr != SendBuf_Ok)
    {
        mState = SessionState_Closed;
        return Session_SendBufferError;
    }

    eRecvBufferError rErr = mRecvBuffer.Open();
    if (rErr != RecvBuf_Ok)
    {
        mState = SessionState_Closed;
        return Session_RecvBufferError;
    }

    if (!mSocket.IsOpen())
    {
        mState = SessionState_Closed;
        return Session_SocketError;
    }

    mState = SessionState_Open;
    mLastActive = std::chrono::steady_clock::now();
    return Session_Ok;
}

void Session::Close()
{
    if (mState == SessionState_Closed)
    {
        return;
    }

    mState = SessionState_Closing;

    mSocket.Close();
    mRecvBuffer.Close();
    mSendBuffer.Close();

    mState = SessionState_Closed;

    InvokeCloseCallback();
}

bool Session::IsOpen() const
{
    return mState == SessionState_Open && mSocket.IsOpen();
}

eSessionError Session::PollRecv()
{
    if (!IsOpen())
    {
        return Session_NotOpen;
    }

    std::uint8_t tmpBuf[4096];
    size_t received = 0;
    eSocketError err = mSocket.Recv(tmpBuf, sizeof(tmpBuf), received);
    if (err != Socket_Ok)
    {
        if (err == Socket_RecvFailed)
        {
            return Session_Ok;
        }

        Close();
        return Session_SocketError;
    }

    if (received == 0)
    {
        Close();
        return Session_PeerClosed;
    }

    size_t written = 0;
    eRecvBufferError rbErr = mRecvBuffer.Write(tmpBuf, received, written);
    if (rbErr != RecvBuf_Ok || written != received)
    {
        Close();
        return Session_RecvBufferError;
    }
    mLastActive = std::chrono::steady_clock::now();

    InvokeRecvCallback();

    return Session_Ok;
}

eSessionError Session::FlushSend()
{
    if (!IsOpen())
    {
        return Session_NotOpen;
    }

    if (!mSendBuffer.IsOpen())
    {
        return Session_SendBufferError;
    }

    if (mSendBuffer.IsEmpty())
    {
        return Session_Ok;
    }

    std::uint8_t tmpBuf[4096];
    while (!mSendBuffer.IsEmpty())
    {
        size_t toSend = 0;
        eSendBufferError sbErr = mSendBuffer.Read(tmpBuf, sizeof(tmpBuf), toSend);

        if (sbErr != SendBuf_Ok || toSend == 0)
        {
            break;
        }
        if (sbErr != SendBuf_Ok)
        {
            return Session_SendBufferError;
        }

        size_t sent = 0;
        eSocketError sErr = mSocket.Send(tmpBuf, toSend, sent);
        if (sErr != Socket_Ok)
        {
            Close();
            return Session_SocketError;
        }

        InvokeSendCallback(sent);
    }

    mLastActive = std::chrono::steady_clock::now();
    return Session_Ok;
}

int Session::Fd() const
{
    return mSocket.GetFd();
}

eSessionState Session::State() const
{
    return mState;
}