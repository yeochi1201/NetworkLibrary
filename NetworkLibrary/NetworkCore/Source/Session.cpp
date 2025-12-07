#include "Session.h"

Session::Session(size_t recvBufSize, size_t sendBufSize, Socket &&socket)
    : mSocket(std::move(socket)), mRecvBuffer(recvBufSize), mSendBuffer(sendBufSize), mState(SessionState_Closed), mLastActive(std::chrono::steady_clock::now())
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
    if (mState == SessionState_Open || mState == SessionState_Opening)
        return Session_AlreadyOpen;

    mState = SessionState_Opening;

    eRecvBufferError rErr = mRecvBuffer.Open();
    if (rErr != RecvBuf_Ok)
    {
        mState = SessionState_Closed;
        return Session_RecvBufferError;
    }

    eSendBufferError sErr = mSendBuffer.Open();
    if (sErr != SendBuf_Ok)
    {
        mRecvBuffer.Close();
        mState = SessionState_Closed;
        return Session_SendBufferError;
    }

    if (!mSocket.IsOpen())
    {
        mRecvBuffer.Close();
        mSendBuffer.Close();
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
eSessionError Session::QueueSend(const void *data, size_t len)
{
    if (!IsOpen())
    {
        return Session_NotOpen;
    }

    if (!mSendBuffer.IsOpen())
    {
        return Session_SendBufferError;
    }

    if (len == 0)
    {
        return Session_Ok;
    }

    if (data == nullptr)
    {
        return Session_InvalidArgs;
    }

    size_t written = 0;
    eSendBufferError sbErr = mSendBuffer.Write(data, len, written);
    if (sbErr != SendBuf_Ok || written != len)
    {
        return Session_SendBufferError;
    }

    mLastActive = std::chrono::steady_clock::now();
    return Session_Ok;
}

eSessionError Session::OnReadable()
{
    if (!IsOpen())
    {
        return Session_NotOpen;
    }

    std::uint8_t tmpBuf[4096];
    for (;;)
    {
        size_t received = 0;
        eSocketError err = mSocket.Recv(tmpBuf, sizeof(tmpBuf), received);
        if (err == Socket_WouldBlock)
        {
            break;
        }
        if (err != Socket_Ok)
        {
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
    }
    InvokeRecvCallback();
    return Session_Ok;
}
eSessionError Session::OnWritable()
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
    for (;;)
    {
        if (mSendBuffer.IsEmpty())
        {
            break;
        }
        size_t peeked = 0;
        eSendBufferError sbErr = mSendBuffer.Peek(tmpBuf, sizeof(tmpBuf), peeked);
        if (sbErr != SendBuf_Ok)
        {
            return Session_SendBufferError;
        }
        if (peeked == 0)
        {
            break;
        }

        size_t sent = 0;
        eSocketError sErr = mSocket.Send(tmpBuf, peeked, sent);
        if (sErr == Socket_WouldBlock)
        {
            // send at next epollout
            break;
        }

        if (sErr != Socket_Ok)
        {
            Close();
            return Session_SocketError;
        }

        if (sent > 0)
        {
            eSendBufferError cErr = mSendBuffer.Consume(sent);
            if (cErr != SendBuf_Ok)
            {
                Close();
                return Session_SendBufferError;
            }

            InvokeSendCallback(sent);
            mLastActive = std::chrono::steady_clock::now();
        }
    }
    return Session_Ok;
}

void Session::SetRecvCallback(RecvCallback callback)
{
    mRecvCallback = std::move(callback);
}
void Session::SetSendCallback(SendCallback callback)
{
    mSendCallback = std::move(callback);
}
void Session::SetCloseCallback(CloseCallback callback)
{
    mCloseCallback = std::move(callback);
}

int Session::Fd() const
{
    return mSocket.GetFd();
}
eSessionState Session::State() const
{
    return mState;
}

RecvBuffer &Session::RecvBuf() noexcept
{
    return mRecvBuffer;
}
const RecvBuffer &Session::RecvBuf() const noexcept
{
    return mRecvBuffer;
}

SendBuffer &Session::SendBuf() noexcept
{
    return mSendBuffer;
}
const SendBuffer &Session::SendBuf() const noexcept
{
    return mSendBuffer;
}

std::chrono::steady_clock::time_point &Session::LastActiveTime() noexcept
{
    return mLastActive;
}

void Session::InvokeRecvCallback()
{
    if (mRecvCallback)
    {
        mRecvCallback(*this, mRecvBuffer);
    }
}
void Session::InvokeSendCallback(size_t sentBytes)
{
    if (mSendCallback)
    {
        mSendCallback(*this, sentBytes);
    }
}
void Session::InvokeCloseCallback()
{
    if (mCloseCallback)
    {
        mCloseCallback(*this);
    }
}