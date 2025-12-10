#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "logger.h"

// 定义模块名为 "TEST"
#define TEST_INFO(...)  SX_LOG_INFO("TEST", __VA_ARGS__)
#define TEST_ERROR(...) SX_LOG_ERROR("TEST", __VA_ARGS__)

TEST(LoggerTest, BasicLogging) { // NOLINT
    sx::LogManager::init("test_log.log");
    SX_LOG_INFO("TestModule", "This is an info log: {}", 42);
    SX_LOG_ERROR("TestModule", "This is an error log: {}", "error occurred");
    SX_LOG_DEBUG("TestModule", "This is a debug log");
    SX_LOG_WARN("TestModule", "This is a warning log");
    SX_LOG_TRACE("TestModule", "This is a trace log");
}

TEST(LoggerTest, ModuleLogging) { // NOLINT
    sx::LogManager::init("test_log.log");
    TEST_INFO("Info log from TEST module: {}", 3.14);
    TEST_ERROR("Error log from TEST module: {}", "critical failure");
}
