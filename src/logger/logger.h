#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "fmt/format.h"

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

// 模板格式化并记录（使用 fmt，提供编译期格式检查）
template <typename... Args>
inline void log_wrapper(const std::string& module_name,
                        LogLevel level,
                        const char* file,
                        int line,
                        fmt::format_string<Args...> fmt_str,
                        Args&&... args) {
    std::string msg = fmt::format(fmt_str, std::forward<Args>(args)...);
    log_internal(module_name, level, file, line, msg);
}

}  // namespace sx

#define SX_LOG_INFO(module, ...) \
    sx::log_wrapper(module, sx::LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define SX_LOG_ERROR(module, ...) \
    sx::log_wrapper(module, sx::LogLevel::ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define SX_LOG_DEBUG(module, ...) \
    sx::log_wrapper(module, sx::LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define SX_LOG_WARN(module, ...) \
    sx::log_wrapper(module, sx::LogLevel::WARN, __FILE__, __LINE__, __VA_ARGS__)
#define SX_LOG_TRACE(module, ...) \
    sx::log_wrapper(module, sx::LogLevel::TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define SX_LOG_CRITICAL(module, ...) \
    sx::log_wrapper(module, sx::LogLevel::CRITICAL, __FILE__, __LINE__, __VA_ARGS__)
