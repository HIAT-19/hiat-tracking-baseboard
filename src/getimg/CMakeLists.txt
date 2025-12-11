cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3")

set(PROJECT_NAME "main")
set(SDK_ROOT_PATH "/home/tronlong/RK3588/rk3588_linux_release_v1.2.1")


add_compile_options(
    "$<$<COMPILE_LANG_AND_ID:C,GNU>:-march=armv8.2-a;-mtune=cortex-a76;-funroll-loops;-finline-functions;-finline-small-functions;-ftree-vectorize>"
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-march=armv8.2-a;-mtune=cortex-a76;-funroll-loops;-finline-functions;-finline-small-functions;-ftree-vectorize>"
)




# 加载子模块
add_subdirectory(src)
# add_subdirectory(test)




### UTs ###
add_executable(getimg_test ${CMAKE_CURRENT_LIST_DIR}/test/getimg_test.c)
target_link_libraries(getimg_test 
    PRIVATE
        getimg
)
# AddTests(getimg_test)