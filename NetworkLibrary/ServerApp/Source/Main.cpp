#include <array>
#include <cstring>
#include <iostream>

#include "ListenerSocket.h"
#include "Socket.h"

int main()
{
    ListenerSocket listener(3000);

    if (listener.Open() != ListenerSocket_Ok)
    {
        std::cerr << "Listener open failed\n";
        return 1;
    }

    Socket clientSocket;
    if (listener.Accept(clientSocket) != ListenerSocket_Ok)
    {
        std::cerr << "Accept failed\n";
        return 1;
    }

    std::array<char, 1024> buffer{};

    for (;;)
    {
        std::size_t received = 0;
        eSocketError recvErr =
            clientSocket.Recv(buffer.data(), buffer.size() - 1, received);

        if (recvErr != Socket_Ok || received == 0)
        {
            std::cout << "connection closed or recv error\n";
            break;
        }

        buffer[received] = '\0';
        std::cout << "[server] recv: " << buffer.data() << '\n';

        std::size_t sent = 0;
        eSocketError sendErr =
            clientSocket.Send(buffer.data(), received, sent);

        if (sendErr != Socket_Ok)
        {
            std::cout << "send error\n";
            break;
        }
    }

    return 0;
}
