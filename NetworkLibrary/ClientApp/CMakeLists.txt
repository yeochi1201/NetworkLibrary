# ClientApp/CMakeLists.txt

add_executable(ClientApp
    Source/Main.cpp
    Source/EpollClient.cpp
)

target_include_directories(ClientApp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Header
)

target_link_libraries(ClientApp
    PRIVATE
        NetworkCore
)

set_target_properties(ClientApp PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
