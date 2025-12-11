#include "logger.h"

#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <mutex>
#include <string>
#include <memory>
#include <chrono>

#include "spdlog/logger.h"

namespace sx
{

// 内部管理结构
struct LogManagerImpl
{
    std::vector<spdlog::sink_ptr> sinks;
    std::mutex mutex;
} __attribute__((aligned(64)));

namespace
{
auto get_impl() -> LogManagerImpl& {
    static LogManagerImpl impl;
    return impl;
}
}  // anonymous namespace

void LogManager::init(const std::string& log_file_path) {
    auto& impl = get_impl();

    // 1. 控制台 Sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // 2. 文件 Sink (滚动日志)
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file_path, 1024 * 1024 * 10, 3);  // 10MB, 3 files

    impl.sinks = {console_sink, file_sink};

    // 设置全局刷新策略
    spdlog::flush_every(std::chrono::seconds(3));
}

auto LogManager::get_logger(const std::string& module_name) -> std::shared_ptr<void> {
    auto logger = spdlog::get(module_name);
    if (logger) {
        return logger;
    }

    // 如果不存在，创建一个新的 logger 并绑定 sinks
    auto& impl = get_impl();
    std::scoped_lock const lock(impl.mutex);

    // Double check
    logger = spdlog::get(module_name);
    if (logger) { return logger;
}

    // 创建 logger (推荐使用 factory 模式或手动组装)
    logger = std::make_shared<spdlog::logger>(module_name, impl.sinks.begin(), impl.sinks.end());

    // 默认级别 (可以通过配置文件加载不同模块的级别)
    logger->set_level(spdlog::level::info);

    // 注册到全局仓库
    spdlog::register_logger(logger);

    return logger;
}

// 将自定义 Level 映射到 spdlog level
static auto to_spdlog_level(LogLevel level) -> spdlog::level::level_enum {
    switch (level) {
        case LogLevel::INFO:
            return spdlog::level::info;
        case LogLevel::ERROR:
            return spdlog::level::err;
        case LogLevel::DEBUG:
            return spdlog::level::debug;
        case LogLevel::WARN:
            return spdlog::level::warn;
        case LogLevel::TRACE:
            return spdlog::level::trace;
        case LogLevel::CRITICAL:
            return spdlog::level::critical;
        case LogLevel::OFF:
            return spdlog::level::off;
        default:
            return spdlog::level::info;
    }
}

void log_internal(const std::string& module_name,
                  LogLevel level,
                  const char* file,
                  int line,
                  const std::string& msg) {
    auto logger_ptr = std::static_pointer_cast<spdlog::logger>(LogManager::get_logger(module_name));

    if (logger_ptr) {
        // spdlog 提供了 source_loc 结构体来接收文件行号
        logger_ptr->log(spdlog::source_loc{file, line, SPDLOG_FUNCTION}, to_spdlog_level(level),
                        msg);
    }
}

}  // namespace sx
