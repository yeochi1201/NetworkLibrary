#include "EpollServer.h"

int main()
{
    EpollServer server(8080, 64 * 1024, 64 * 1024); // recv/send buf size 예시

    if (!server.Start())
        return 1;

    server.Run();
    return 0;
}
