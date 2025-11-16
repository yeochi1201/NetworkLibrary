# NetworkCore/CMakeLists.txt

add_library(NetworkCore STATIC
    Source/ListenerSocket.cpp
    Source/Socket.cpp
    Header/ListenerSocket.h
    Header/Socket.h
)

target_include_directories(NetworkCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Header
)

set_target_properties(NetworkCore PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)