cmake_minimum_required(VERSION 3.16)

project(NetworkLibrary LANGUAGES CXX)

add_subdirectory(NetworkCore)
add_subdirectory(ServerApp)
add_subdirectory(ClientApp)