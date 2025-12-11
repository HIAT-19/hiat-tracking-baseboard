add_executable(hd_mipi_cap_test
    hd_mipi_cap_test.cpp
)

target_link_libraries(hd_mipi_cap_test
    PRIVATE
        hd_mipi_cap
)
