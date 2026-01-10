#include "EpollServer.h"
#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>

EpollServer::EpollServer(uint16_t port, size_t recvBufSize, size_t sendBufSize)
    : mEpollFd(-1), mRunning(false), mListener(port, 100),
      mRecvBufSize(recvBufSize), mSendBufSize(sendBufSize) {}

EpollServer::~EpollServer() {
  Stop();
  if (mEpollFd != -1) {
    close(mEpollFd);
    mEpollFd = -1;
  }
}

bool EpollServer::Start() {
  if (mListener.Open() != ListenerSocket_Ok) {
    std::cerr << "Listener open failed\n";
    return false;
  }

  mEpollFd = ::epoll_create1(0);
  if (mEpollFd < 0) {
    std::perror("epoll_create1");
    return false;
  }

  epoll_event ev{};
  ev.events = EPOLLIN;
  ev.data.fd = mListener.GetFd();

  if (::epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mListener.GetFd(), &ev) < 0) {
    std::perror("epoll_ctl ADD listener");
    return false;
  }

  mRunning = true;
  return true;
}
void EpollServer::UpdateWriteInterest(int fd, bool enable) {
  epoll_event ev{};
  ev.data.fd = fd;
  ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
  if (enable)
    ev.events |= EPOLLOUT;

  ::epoll_ctl(mEpollFd, EPOLL_CTL_MOD, fd, &ev);
}

void EpollServer::HandleNewConnection() {
  for (;;) {
    Socket clientSocket;
    eListenerSocketError acceptErr = mListener.Accept(clientSocket);
    if (acceptErr == ListenerSocket_AcceptFailed) {
      break;
    } else if (acceptErr != ListenerSocket_Ok) {
      std::cerr << "Accept failed\n";
      break;
    }

    clientSocket.SetBlocking(false);
    auto session = std::make_unique<Session>(mRecvBufSize, mSendBufSize,
                                             std::move(clientSocket));
    if (session->Open(mRecvBufSize, mSendBufSize) != Session_Ok) {
      continue;
    }

    int fd = session->Fd();

    // Common TCP/IP Server
    // session->SetRecvCallback([](Session& s, RecvBuffer& rb){
    //     //TODO Handle RecvCallback
    //     std::uint8_t buf[4096];

    //     while (!rb.IsEmpty())
    //     {
    //         std::size_t readBytes = 0;
    //         eRecvBufferError err = rb.Read(buf, sizeof(buf), readBytes);
    //         if (err != RecvBuf_Ok || readBytes == 0)
    //             break;

    //         eSessionError se = s.QueueSend(buf, readBytes);
    //         if (se != Session_Ok)
    //         {
    //             break;
    //         }
    //     }
    // });

    // HTTP Server
    session->SetRecvCallback([this](Session &s, RecvBuffer &rb) {
      auto &st = mHttpStates[s.Fd()];

      while (true) {
        HttpRequest req;
        std::string perr;

        HttpParser::Result r = st.parser.TryParse(rb, req, &perr);

        if (r == HttpParser::Result::Http_NeedMore)
          break;

        if (r == HttpParser::Result::Http_Error) {
          HttpResponse resp;
          resp.status = 400;
          resp.reason = "Bad Request";
          resp.SetTextBody("bad request");

          auto bytes = BuildHttpResponseBytes(resp, /*keepAlive=*/false);
          (void)s.QueueSend(bytes.data(), bytes.size());

          st.closeAfterSend = true;
          break;
        }

        // ---------- Connection: close 처리 ----------
        bool keepAlive = true;
        if (auto c = req.Header("connection")) {
          std::string v(*c);
          for (auto &ch : v)
            ch = (char)std::tolower((unsigned char)ch);
          if (v.find("close") != std::string::npos) {
            keepAlive = false;
            st.closeAfterSend = true;
          }
        }
        // HTTP/1.0 기본 close 정책까지 반영하고 싶으면:
        // if (req.version == "HTTP/1.0") keepAlive = false;

        // ---------- 라우팅 (/health, /echo) ----------
        HttpResponse resp;

        if (req.method == "GET" && req.target == "/health") {
          resp.status = 200;
          resp.reason = "OK";
          resp.SetTextBody("ok");
        } else if (req.method == "POST" && req.target == "/echo") {
          resp.status = 200;
          resp.reason = "OK";
          resp.headers["Content-Type"] = "application/octet-stream";
          resp.body = std::move(req.body);
        } else if (req.method == "GET" &&
                   req.target.rfind("/echo?msg=", 0) == 0) {
          std::string msg = req.target.substr(std::string("/echo?msg=").size());
          resp.status = 200;
          resp.reason = "OK";
          resp.SetTextBody(msg);
        } else {
          resp.status = 404;
          resp.reason = "Not Found";
          resp.SetTextBody("not found");
        }

        auto bytes = BuildHttpResponseBytes(resp, keepAlive);
        (void)s.QueueSend(bytes.data(), bytes.size());

        // 루프 계속 → 같은 recv 덩어리 안에 다음 요청이 붙어왔으면 계속 파싱
        // 가능
        continue;
      }
    });

    // HTTP Server
    session->SetSendCallback([this](Session &s, size_t /*sent*/) {
      auto it = mHttpStates.find(s.Fd());
      if (it == mHttpStates.end())
        return;

      // 마지막 바이트를 보낸 순간 sent 콜백이 오므로 여기서 비었는지 검사 가능
      if (it->second.closeAfterSend && !s.HasPendingSend()) {
        s.Close();
      }
    });
    session->SetCloseCallback([this](Session &s) {
      int fd = s.Fd();
      ::epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, nullptr);
      mSessions.erase(fd);

      //HTTP Server
      mHttpStates.erase(fd);
    });

    // Common TCP/IP Server
    // session->SetFrameCallback([](Session &s, const std::uint8_t *p,
    //                              std::size_t n) { s.SendFrame(p, n); });
    // session->SetWriteInterestCallback([this](Session &s, bool enable) {
    //   this->UpdateWriteInterest(s.Fd(), enable);
    // });

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    ev.data.fd = session->Fd();

    if (::epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &ev) < 0) {
      std::perror("epoll_ctl ADD client");
      continue;
    }

    mSessions.emplace(fd, std::move(session));
  }
}

void EpollServer::HandleClientEvent(int fd, uint32_t events) {
  auto it = mSessions.find(fd);
  if (it == mSessions.end())
    return;

  Session &sess = *(it->second);

  if (events & (EPOLLERR | EPOLLHUP)) {
    sess.Close();
    return;
  }

  if (events & EPOLLIN) {
    eSessionError r = sess.OnReadable();
    if (r == Session_PeerClosed || r == Session_SocketError ||
        r == Session_RecvBufferError) {
      return;
    }
  }

  if (events & EPOLLOUT) {
    eSessionError w = sess.OnWritable();
    if (w == Session_SocketError) {
      return;
    }
  }
}

void EpollServer::Run() {
  constexpr int MAX_EVENTS = 64;
  epoll_event events[MAX_EVENTS];

  constexpr auto kIdleTimeout = std::chrono::seconds(30);
  constexpr int kTickMs = 1000;

  while (mRunning) {
    int n = ::epoll_wait(mEpollFd, events, MAX_EVENTS, kTickMs);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      std::perror("epoll_wait");
      break;
    }

    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;
      uint32_t ev = events[i].events;

      if (fd == mListener.GetFd()) {
        HandleNewConnection();
      } else {
        HandleClientEvent(fd, ev);
      }
    }

    const auto now = std::chrono::steady_clock::now();
    for (auto it = mSessions.begin(); it != mSessions.end();) {
      Session &s = *(it->second);
      if (s.IsIdleTimeout(std::chrono::duration_cast<std::chrono::milliseconds>(
              kIdleTimeout))) {
        auto victim = it++;
        victim->second->Close();
        continue;
      }

      it++;
    }
  }
}

void EpollServer::Stop() {
  mRunning = false;
  if (mEpollFd >= 0) {
    ::close(mEpollFd);
    mEpollFd = -1;
  }
  mListener.Close();
  mSessions.clear();
}
