#include <array>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ListenerSocket.h" // 굳이 안 써도 되지만 include는 가능
#include "Socket.h"

int main()
{
    // 1) raw fd로 TCP 소켓 생성
    int sockFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0)
    {
        std::perror("socket");
        return 1;
    }

    // 2) 서버에 연결 (127.0.0.1:3000)
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

    // 3) fd를 Socket 클래스로 감싸기
    Socket socket(sockFd);

    std::string line;
    std::array<char, 1024> buffer{};

    while (std::getline(std::cin, line))
    {
        line.push_back('\n');

        std::size_t sent = 0;
        if (socket.Send(line.data(), line.size(), sent) != Socket_Ok)
        {
            std::cout << "send error\n";
            break;
        }

        std::size_t received = 0;
        if (socket.Recv(buffer.data(), buffer.size() - 1, received) != Socket_Ok || received == 0)
        {
            std::cout << "recv error or closed\n";
            break;
        }

        buffer[received] = '\0';
        std::cout << "[client] echo: " << buffer.data();
    }

    return 0;
}
