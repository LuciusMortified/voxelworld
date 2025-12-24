#pragma once

#ifndef VW_LOG_LOGGER_INL_H
#define VW_LOG_LOGGER_INL_H

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <format>
#include <memory>


namespace vw::log {

class logger::impl {
public:
    impl() {
        logger_ = spdlog::stdout_color_mt("vwengine");
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        logger_->set_level(spdlog::level::trace);
    }

    ~impl() = default;

    template <typename... Args>
    void trace(
        std::format_string<Args...> fmt, Args&&... args
    ) {
        logger_->trace(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void trace(
        log_category cat, std::format_string<Args...> fmt, Args&&... args
    ) {
        if (cat.value.empty()) {
            logger_->trace(std::format(fmt, std::forward<Args>(args)...));
        } else {
            logger_->trace("[{}] {}", cat.value, std::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    void debug(
        std::format_string<Args...> fmt, Args&&... args
    ) {
        logger_->debug(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void debug(
        log_category cat, std::format_string<Args...> fmt, Args&&... args
    ) {
        if (cat.value.empty()) {
            logger_->debug(std::format(fmt, std::forward<Args>(args)...));
        } else {
            logger_->debug("[{}] {}", cat.value, std::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    void info(
        std::format_string<Args...> fmt, Args&&... args
    ) {
        logger_->info(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void info(
        log_category cat, std::format_string<Args...> fmt, Args&&... args
    ) {
        if (cat.value.empty()) {
            logger_->info(std::format(fmt, std::forward<Args>(args)...));
        } else {
            logger_->info("[{}] {}", cat.value, std::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    void warn(
        std::format_string<Args...> fmt, Args&&... args
    ) {
        logger_->warn(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void warn(
        log_category cat, std::format_string<Args...> fmt, Args&&... args
    ) {
        if (cat.value.empty()) {
            logger_->warn(std::format(fmt, std::forward<Args>(args)...));
        } else {
            logger_->warn("[{}] {}", cat.value, std::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    void error(
        std::format_string<Args...> fmt, Args&&... args
    ) {
        logger_->error(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void error(
        log_category cat, std::format_string<Args...> fmt, Args&&... args
    ) {
        if (cat.value.empty()) {
            logger_->error(std::format(fmt, std::forward<Args>(args)...));
        } else {
            logger_->error("[{}] {}", cat.value, std::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    void critical(
        std::format_string<Args...> fmt, Args&&... args
    ) {
        logger_->critical(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void critical(
        log_category cat, std::format_string<Args...> fmt, Args&&... args
    ) {
        if (cat.value.empty()) {
            logger_->critical(std::format(fmt, std::forward<Args>(args)...));
        } else {
            logger_->critical("[{}] {}", cat.value, std::format(fmt, std::forward<Args>(args)...));
        }
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};

inline logger::logger() : pimpl_(new impl()) {}

inline logger::~logger() {
    delete pimpl_;
}

inline logger& logger::get() {
    static logger instance;
    return instance;
}

template <typename... Args>
void logger::trace(
    std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->trace(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logger::trace(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->trace(cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logger::debug(
    std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->debug(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logger::debug(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->debug(cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logger::info(
    std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->info(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logger::info(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->info(cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logger::warn(
    std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->warn(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logger::warn(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->warn(cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logger::error(
    std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->error(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logger::error(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->error(cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logger::critical(
    std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->critical(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void logger::critical(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    pimpl_->critical(cat, fmt, std::forward<Args>(args)...);
}

// Функции-обертки в namespace vw::log
template <typename... Args>
void trace(
    std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().trace(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void trace(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().trace(cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(
    std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().debug(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().debug(cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(
    std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().info(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().info(cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(
    std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().warn(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().warn(cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(
    std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().error(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().error(cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void critical(
    std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().critical(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void critical(
    log_category cat, std::format_string<Args...> fmt, Args&&... args
) {
    logger::get().critical(cat, fmt, std::forward<Args>(args)...);
}

}  // namespace vw::log

#endif  // VW_LOG_LOGGER_INL_H
