#include <array>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Socket.h"
#include "Session.h"

int main()
{

    int sockFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0)
    {
        std::perror("socket");
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = ::htons(3000);

    if (::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0)
    {
        std::perror("inet_pton");
        ::close(sockFd);
        return 1;
    }

    if (::connect(sockFd,
                  reinterpret_cast<sockaddr *>(&serverAddr),
                  sizeof(serverAddr)) < 0)
    {
        std::perror("connect");
        ::close(sockFd);
        return 1;
    }

    Socket socket(sockFd);
    Session session(4096, 4096, std::move(socket));

    if (session.Open(4096, 4096) != Session_Ok)
    {
        std::cerr << "Session open failed\n";
        return 1;
    }

    session.SetRecvCallback(
        [](Session &, RecvBuffer &buf)
        {
            char tmp[1024];

            while (!buf.IsEmpty())
            {
                std::size_t read = 0;
                eRecvBufferError err = buf.Read(tmp, sizeof(tmp) - 1, read);
                if (err != RecvBuf_Ok || read == 0)
                    break;

                tmp[read] = '\0';
                std::cout << "[client] echo: " << tmp;
            }
        });

    std::string line;

    while (std::getline(std::cin, line))
    {
        line.push_back('\n');

        eSessionError qErr = session.QueueSend(line.data(), line.size());
        if (qErr != Session_Ok)
        {
            std::cout << "QueueSend error\n";
            break;
        }

        eSessionError fErr = session.FlushSend();
        if (fErr != Session_Ok)
        {
            std::cout << "send error\n";
            break;
        }

        eSessionError rErr = session.PollRecv();
        if (rErr == Session_PeerClosed)
        {
            std::cout << "recv closed by peer\n";
            break;
        }
        if (rErr != Session_Ok)
        {
            std::cout << "recv error\n";
            break;
        }
    }

    return 0;
}
