#add_library(img_xdma STATIC img_xdma.c)


cmake_minimum_required(VERSION 3.10)

add_library(img_local STATIC
    img_local.c
    img_local.h
)

# 关键：链接queue库（PUBLIC，因为img_xdma的头文件依赖queue.h）
target_link_libraries(img_local
    PUBLIC queue  # 用PUBLIC，让依赖img_xdma的img_get也继承queue的头文件路径
)

# 暴露img_xdma自己的头文件路径（让img_get能找到img_xdma.h）
target_include_directories(img_local
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(img_local PRIVATE -g -O3)
