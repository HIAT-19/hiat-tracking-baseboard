


cmake_minimum_required(VERSION 3.10)

add_executable(test_main main.c)

# 只需链接业务模块，queue会自动被链接进来
target_link_libraries(test_main
    PRIVATE
    img_get
)