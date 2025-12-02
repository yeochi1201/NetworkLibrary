#include <array>
#include <cstring>
#include <iostream>

#include "ListenerSocket.h"
#include "Socket.h"
#include "Session.h"

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

    // ---- 여기부터 Session 사용 ----
    // recvBufSize, sendBufSize는 예시로 4096
    Session session(4096, 4096, std::move(clientSocket));

    if (session.Open(4096, 4096) != Session_Ok)
    {
        std::cerr << "Session open failed\n";
        return 1;
    }

    // Recv 콜백: RecvBuffer에 들어온 데이터를 모두 읽어서 그대로 에코
    session.SetRecvCallback(
        [](Session &s, RecvBuffer &buf)
        {
            char tmp[1024];

            while (!buf.IsEmpty())
            {
                std::size_t read = 0;
                eRecvBufferError rbErr = buf.Read(tmp, sizeof(tmp) - 1, read);
                if (rbErr != RecvBuf_Ok || read == 0)
                    break;

                tmp[read] = '\0';
                std::cout << "[server] recv: " << tmp;

                // 그대로 다시 보내기
                eSessionError qErr = s.QueueSend(tmp, read);
                if (qErr != Session_Ok)
                {
                    std::cout << "QueueSend error\n";
                    break;
                }
            }
        });

    session.SetSendCallback(
        [](Session &s, std::size_t sentBytes)
        {
            std::cout << "[server] sent " << sentBytes << " bytes\n";
        });

    for (;;)
    {
        // 소켓에서 Recv → RecvBuffer에 누적 → OnRecv 호출
        eSessionError rErr = session.PollRecv();
        if (rErr == Session_PeerClosed)
        {
            std::cout << "connection closed by peer\n";
            break;
        }
        if (rErr != Session_Ok)
        {
            std::cout << "recv error\n";
            break;
        }

        // SendBuffer에 쌓여 있는 데이터 → 소켓으로 실제 송신
        eSessionError sErr = session.FlushSend();
        if (sErr != Session_Ok)
        {
            std::cout << "send error\n";
            break;
        }
    }

    return 0;
}
