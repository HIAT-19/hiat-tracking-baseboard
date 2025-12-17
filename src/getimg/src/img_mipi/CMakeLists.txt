cmake_minimum_required(VERSION 3.10)
add_library(img_mipi STATIC
    hd_mipi_cap.cpp
    buffer.c
    img_mipi.cpp
)
# 关键：链接queue库（PUBLIC，因为img_xdma的头文件依赖queue.h）
target_link_libraries(img_mipi
    PUBLIC queue  # 用PUBLIC，让依赖img_xdma的img_get也继承queue的头文件路径
)
target_include_directories(img_mipi
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
