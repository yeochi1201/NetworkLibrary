# ClientApp/CMakeLists.txt

add_executable(ClientApp
    Source/Main.cpp
)

target_link_libraries(ClientApp
    PRIVATE
        NetworkCore
)

set_target_properties(ClientApp PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
