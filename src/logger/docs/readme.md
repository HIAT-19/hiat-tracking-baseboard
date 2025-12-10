# simple logger 

基于spdlog的简单封装
该日志服务系统需要指定模块进行打印，便于区分打印模块

使用方法：
```c++
// 假如模块名为 "XXX"
#define XXX_INFO(...)  SX_LOG_INFO("TEST", __VA_ARGS__)
#define XXX_ERROR(...) SX_LOG_ERROR("TEST", __VA_ARGS__)
// ... 其他使用打印级别

// Init
LogManager::init("app.log");    // 仅需全局初始化一次
// 打印
TEST_INFO("Info log from TEST module: {}", 3.14);
TEST_ERROR("Error log from TEST module: {}", "critical failure");

```

TODO：增加只开启单模块功能
