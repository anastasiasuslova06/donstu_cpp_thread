// threadfuncs.cpp
#include <thread>
#include "threadfuncs.h"

#include <atomic>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <syscall.h>
#include <sys/types.h>
#include <future>

// определение глобального атомарного счётчика (задание 20)
std::atomic<int> counter{0};

Logger::Logger(const std::string& filename)
    : file_(filename, std::ios::out | std::ios::trunc)
{
    if (!file_.is_open()) {
        throw std::runtime_error("Cannot open log file: " + filename);
    }
}

Logger::~Logger() {
    // Файл закрывается автоматически в деструкторе std::ofstream (RAII)
}

bool Logger::writeLine(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_ << msg << '\n';
    file_.flush();
    return static_cast<bool>(file_);
}

pid_t getThreadID() {
    return static_cast<pid_t>(::syscall(SYS_gettid));
    // для Windows: return GetCurrentThreadId();
}

void about() {
    std::cout << "std::thread example\n";
}

void funcThread(const ThreadArgs& args,
                Logger& logger,
                std::promise<std::string> result) {
    // 100000 инкрементов на поток — один раз (задание 20)
    for (int k = 0; k < 100000; ++k) {
        ++counter;
    }

    // основной цикл логирования
    for (int i = 0; i < COUNT_ITERATIONS; ++i) {
        std::ostringstream oss;
        oss << "[tag = " << args.tag
            << "] pid = " << ::getpid()
            << " ppid = " << ::getppid()
            << " tid = " << getThreadID()
            << " std::thread::id = " << std::this_thread::get_id()
            << " iter = " << i;
        logger.writeLine(oss.str());

        // imitation of useful work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // формируем результат и передаём через promise
    std::ostringstream res;
    res << args.tag << ": " << COUNT_ITERATIONS << " iterations done";
    result.set_value(res.str());
}
