#include "EpollClient.h"

int main()
{
    EpollClient client("127.0.0.1", 3000, 64 * 1024, 64 * 1024);

    if (!client.Start())
        return 1;

    const char msg[] = "hello\n";
    client.Send(msg, sizeof(msg) - 1);

    client.Run();
    return 0;
}
