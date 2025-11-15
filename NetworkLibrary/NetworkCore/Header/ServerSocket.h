#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

#include <cstdint>
#include <cstddef>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

typedef enum eServerSockerError
{
    ServerSocket_Ok = 0,
    ServerSocket_InvalidState,
    ServerSocket_SendFailed,
    ServerSocket_RecvFailed
};

class ServerSocket
{
public:
private:
};

#endif