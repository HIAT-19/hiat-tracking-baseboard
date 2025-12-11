

add_library(getimg 
    STATIC 
        img_get/img_get.c
        img_xdma/img_xdma.c
        queue/queue.c
)
target_include_directories(getimg 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}           # 项目根目录
        ${CMAKE_CURRENT_SOURCE_DIR}/img_get   # img_get头文件
        ${CMAKE_CURRENT_SOURCE_DIR}/img_xdma  # img_xdma头文件
        ${CMAKE_CURRENT_SOURCE_DIR}/queue     # queue头文件
)

