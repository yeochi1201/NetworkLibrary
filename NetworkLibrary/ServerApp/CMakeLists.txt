# ServerApp/CMakeLists.txt

add_executable(ServerApp
    Source/Main.cpp
)

target_link_libraries(ServerApp
    PRIVATE
        NetworkCore
)

set_target_properties(ServerApp PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)