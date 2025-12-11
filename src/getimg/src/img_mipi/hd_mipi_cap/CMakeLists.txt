add_library(hd_mipi_cap STATIC
    hd_mipi_cap.cpp
    buffer.c
)

target_include_directories(hd_mipi_cap
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

# add_subdirectory(test)
