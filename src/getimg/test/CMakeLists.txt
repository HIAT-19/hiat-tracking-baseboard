cmake_minimum_required(VERSION 3.10)

add_executable(getimg_test_main getimg_test.c)

# 只需链接业务模块，queue会自动被链接进来
target_link_libraries(getimg_test_main
    PRIVATE
    getimg
)