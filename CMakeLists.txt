project(RemotePinballDisplay)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

add_executable(decode-frame decode-frame.cpp)
add_executable(expand-into-dots expand-into-dots.cpp)
