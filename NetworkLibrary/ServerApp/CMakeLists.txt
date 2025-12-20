# ServerApp/CMakeLists.txt

add_executable(ServerApp
    Source/Main.cpp
    Source/EpollServer.cpp
)

target_include_directories(ServerApp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Header
)

target_link_libraries(ServerApp
    PRIVATE
        NetworkCore
)

set_target_properties(ServerApp PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)