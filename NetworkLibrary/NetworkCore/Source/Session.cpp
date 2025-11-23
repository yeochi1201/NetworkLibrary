#include "Session.h"

Session::Session(size_t recvBufSize, size_t sendBufSize, int socketFd)
    : mSocket(socketFd)
    , mRecvBuffer(recvBufSize)
    , mSendBuffer(sendBufSize)
    , mState(SessionState_Closed)
    , mLastActive(std::chrono::steady_clock::now())
{
}

Session::~Session()
{
    Close();
}

Session::Session(Session&& other) noexcept
    : mSocket(std::move(other.mSocket))
    , mRecvBuffer(std::move(other.mRecvBuffer))
    , mSendBuffer(std::move(other.mSendBuffer))
    , mState(other.mState)
    , mRecvCallback(std::move(other.mRecvCallback))
    , mSendCallback(std::move(other.mSendCallback))
    , mCloseCallback(std::move(other.mCloseCallback))
    , mLastActive(other.mLastActive)
{
    other.mState = SessionState_Closed;
    other.mSendCallback = nullptr;
    other.mRecvCallback = nullptr;
    other.mCloseCallback = nullptr;
}

Session& Session::operator=(Session&& other) noexcept
{
    if (this != &other) {
        mSocket        = std::move(other.mSocket);
        mRecvBuffer    = std::move(other.mRecvBuffer);
        mSendBuffer    = std::move(other.mSendBuffer);
        mState         = other.mState;
        mRecvCallback  = std::move(other.mRecvCallback);
        mSendCallback  = std::move(other.mSendCallback);
        mCloseCallback = std::move(other.mCloseCallback);
        mLastActive    = other.mLastActive;

        other.mState = SessionState_Closed;
        other.mSendCallback = nullptr;
        other.mRecvCallback = nullptr;
        other.mCloseCallback = nullptr;
    }
    return *this;
}