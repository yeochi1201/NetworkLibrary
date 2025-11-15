# CMakeList.txt: CMakeProject1에 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#

# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_executable(MainApp
    "Main.cpp"
    "Main.h"
    "Header/Listener.h"
 "Source/Listener.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET MainApp PROPERTY CXX_STANDARD 20)
endif()

# include 경로는 NetworkCore가 아니라 MainApp에 설정해야 합니다
target_include_directories(MainApp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Header
)

# TODO: 필요한 경우 테스트를 추가하고 대상을 설치합니다.
