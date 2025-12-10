include(spdlog)
include(fmt)

add_library(logger STATIC logger.cpp)
target_include_directories(logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(logger 
    PUBLIC
        fmt::fmt
        spdlog::spdlog_header_only
)

### UTs ###
add_executable(logger_test ${CMAKE_CURRENT_LIST_DIR}/test/logger_test.cpp)
target_link_libraries(logger_test 
    PRIVATE
        logger
)
AddTests(logger_test)
