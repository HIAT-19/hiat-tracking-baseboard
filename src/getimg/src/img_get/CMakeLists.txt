#add_library(img_get STATIC img_get.c)

cmake_minimum_required(VERSION 3.10)

add_library(img_get SHARED
    img_get.c
    img_get.h
)

# 关键：链接img_xdma（PUBLIC/PRIVATE根据需求选，这里PRIVATE即可）
target_link_libraries(img_get
    PRIVATE img_xdma  # 链接img_xdma后，自动继承queue的头文件路径
)

# 若img_get也直接include "queue.h"，可显式加queue（可选，不影响）
# target_link_libraries(img_get PRIVATE img_xdma queue)

# 暴露img_get自己的头文件路径（按需）
target_include_directories(img_get
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(img_get PRIVATE -g -O3)

