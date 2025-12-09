#pragma once

#include <cstdint>
#include <memory>

#include <string>
#include <string_view>

namespace sx
{

enum class LogLevel : uint8_t { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, OFF };

class LogManager
{
public:
    // 初始化日志系统（设置模式、文件路径、异步等）
    static void init(const std::string& log_file_path);

    // 获取特定模块的 Logger 实例
    // 使用 void* 或前向声明隐藏 spdlog::logger 具体类型
    static std::shared_ptr<void> get_logger(const std::string& module_name);
};

// 核心封装函数：用于桥接宏与实现
void log_internal(const std::string& module_name,
                  LogLevel level,
                  const char* file,
                  int line,
                  const std::string& msg);

// 格式化并记录
void log_wrapper(const std::string& module_name,
                 LogLevel level,
                 const char* file,
                 int line,
                 const char* fmt, ...);

}  // namespace sx

#define SX_LOG_INFO(module, ...) \
    sx::log_wrapper(module, LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define SX_LOG_ERROR(module, ...) \
    sx::log_wrapper(module, LogLevel::ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define SX_LOG_DEBUG(module, ...) \
    sx::log_wrapper(module, LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define SX_LOG_WARN(module, ...) \
    sx::log_wrapper(module, LogLevel::WARN, __FILE__, __LINE__, __VA_ARGS__)
#define SX_LOG_TRACE(module, ...) \
    sx::log_wrapper(module, LogLevel::TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define SX_LOG_CRITICAL(module, ...) \
    sx::log_wrapper(module, LogLevel::CRITICAL, __FILE__, __LINE__, __VA_ARGS__)
