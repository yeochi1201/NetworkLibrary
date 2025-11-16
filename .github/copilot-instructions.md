**Project Overview**
- **Purpose:**: Minimal C++ socket/network library with a small server app used for learning/experimentation. The code lives under `NetworkLibrary/`.
- **Major components:**: `NetworkCore/` (core socket primitives), `ServerApp/` (application entrypoint and server orchestration).
- **Platform:**: POSIX/Linux — code includes `<arpa/inet.h>`, `<sys/socket.h>`, `<unistd.h>` and uses POSIX APIs.

**Build & Run**
- **Configure & build:**: From repository root run:

  ```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
  cmake --build build --config Debug
  ```

- **Targets:**: Currently `NetworkCore` is configured as an executable target named `CoreApp` (see `NetworkCore/CMakeLists.txt`). `ServerApp/CMakeLists.txt` appears incomplete; expect to fix CMake if you need `ServerApp` to build as an executable or link `NetworkCore` as a library.

**What to look at first (key files)**
- **Architecture & examples:**: `NetworkCore/Header/Socket.h` and `NetworkCore/Header/ListenerSocket.h` show intended class APIs and usage patterns.
- **Top-level CMake:**: `NetworkLibrary/CMakeLists.txt` includes both subprojects. `NetworkCore/CMakeLists.txt` shows current target `CoreApp` and `target_include_directories` usage.
- **Server scaffold:**: `ServerApp/Header/ServerApp.h` is the app entry class skeleton.

**Codebase conventions & patterns (project-specific)**
- **RAII + explicit resource management:**: Socket classes manage file descriptors and close in destructors (see `Socket::~Socket()` implied). Prefer RAII and explicit `Close()` calls.
- **No copies, move-only types:**: Copy constructors/operators are deleted; move constructors/operators are defined and marked `noexcept`. For example, `ListenerSocket(const ListenerSocket&) = delete; ListenerSocket(ListenerSocket&&) noexcept;`.
- **Error enums:**: Functions return project-specific enum types (e.g., `eSocketError`, `eListenerSocketError`) instead of exceptions. Handle these explicitly. Example: `eListenerSocketError Open();` and `eListenerSocketError Accept(Socket &outServerSocket);`.
- **Member naming:**: member variables use `m` prefix (e.g., `mSocketFd`, `mPort`, `mBacklog`). Keep same style when adding members.
- **Friend usage:**: `Socket` declares `friend class ListenerSocket;` — be cautious when refactoring visibility.

**CMake & project-specific notes**
- **Include directories:**: Comments indicate include paths should be set by the consuming app (MainApp/ServerApp) rather than by `NetworkCore` itself. Follow pattern in `NetworkCore/CMakeLists.txt` where `target_include_directories(CoreApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Header)` is used.
- **Library vs executable:**: `NetworkCore` currently adds `CoreApp` executable. If you want to reuse core code from `ServerApp`, convert `NetworkCore` to a library (`add_library(NetworkCore STATIC|SHARED ...)`) and link it from `ServerApp`.
- **Watch for typos/unfinished CMake:**: `ServerApp/CMakeLists.txt` contains mistakes (empty `add_executable`, stray `f (CMAKE_VERSION ...)`, mismatched `MainApp` target name). An agent making CMake edits should verify and test builds locally.

**Common edits you might make (with examples)**
- Add method implementations in `NetworkCore/Source/*.cpp` for new socket behavior; follow existing method signatures in `Header/*.h`.
- When adding a new public header, add it under `Header/` and update `target_include_directories` where appropriate.
- To make `ServerApp` build, update `ServerApp/CMakeLists.txt` to list sources and set `CXX_STANDARD` for `ServerApp`, or convert `NetworkCore` into a library and link it.

**Debugging & testing hints**
- **Run built binaries:**: built executables will be in `build/` (or `build/Debug` depending on generator). Example: `./build/CoreApp`.
- **Use gdb/lldb:**: run `gdb --args ./build/CoreApp` or use an IDE launch config. Because the code manipulates sockets, tests usually require proper privileges and free ports.
- **No test harness present:**: There are no unit tests in the repo — add small test executables under a `tests/` directory and add to CMake if needed.

**What an agent should always check before changing things**
- Confirm whether `NetworkCore` should be an executable or a library before linking from `ServerApp`.
- Preserve the move-only semantics and existing error enums when modifying APIs.
- Maintain POSIX-only code unless a cross-platform abstraction is intentionally introduced.

**If you find conflicting or missing info**
- If a CMake target is incomplete (like `ServerApp`), update CMake and then run the configure/build steps above. Keep changes minimal and test locally.

**References (examples in repo)**
- `NetworkCore/Header/Socket.h` — socket API and error enums
- `NetworkCore/Header/ListenerSocket.h` — listener pattern and `Accept(Socket &outServerSocket)` usage
- `NetworkCore/CMakeLists.txt` — current target setup and include pattern
- `ServerApp/Header/ServerApp.h` — server application entrypoint

If any section is unclear or you'd like more detail (for example, a suggested small CMake patch to make `NetworkCore` a library and build `ServerApp`), say so and I will prepare a concrete patch.
