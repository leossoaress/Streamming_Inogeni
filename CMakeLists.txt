cmake_minimum_required(VERSION 3.13)
project(Streamming_Inogeni)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++14 -pthread -O0 -Wall")
file(GLOB SRC_FILES "src/*.h" "src/*.cpp")
add_executable(Streamming_Inogeni ${SRC_FILES})

find_package( OpenCV REQUIRED )
target_link_libraries(Streamming_Inogeni ${OpenCV_LIBS})
target_link_libraries(Streamming_Inogeni "-lavdevice -lavformat -lavcodec -lavutil -lswscale")