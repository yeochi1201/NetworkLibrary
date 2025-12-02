# NetworkCore/CMakeLists.txt

add_library(NetworkCore STATIC
    Source/ListenerSocket.cpp
    Header/ListenerSocket.h
    Header/Socket.h
    Source/Socket.cpp
    Header/RecvBuffer.h
    Source/RecvBuffer.cpp
    Header/RingBuffer.h
    Source/RingBuffer.cpp
    Header/SendBuffer.h
    Source/SendBuffer.cpp
    Header/Session.h
    Source/Session.cpp
)

target_include_directories(NetworkCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Header
)

set_target_properties(NetworkCore PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)