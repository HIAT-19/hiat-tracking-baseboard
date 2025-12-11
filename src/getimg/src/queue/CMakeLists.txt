#add_library(queue STATIC queue.c)


cmake_minimum_required(VERSION 3.10)

# 编译queue为静态库（推荐STATIC，也可SHARED动态库）
add_library(queue SHARED
    queue.c
    queue.h  # 仅让IDE识别，编译不影响
)

# 关键：暴露头文件路径（PUBLIC表示依赖queue的模块也能继承这个路径）
target_include_directories(queue
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}  # 外部模块可直接#include "queue.h"
    # 若有私有头文件（仅queue自己用），加PRIVATE 路径
)

target_compile_options(queue PRIVATE -g -O3)
