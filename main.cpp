#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

#include "threadfuncs.h"

int main() {
    about();

    // open log file
    Logger logger("output.log");

    logger.writeLine("main: pid = " + std::to_string(getThreadID())
                     + ", opened file: 'output.log'");

    // args for threads (задание 7 — цикл)
    std::vector<ThreadArgs> args(COUNT_THREADS);
    for (int i = 0; i < COUNT_THREADS; ++i) {
        std::ostringstream oss;
        oss << "T" << i;
        args[i].id  = i;
        args[i].tag = oss.str();
    }

    // threads are starting
    std::vector<std::thread>              threads;
    std::vector<std::future<std::string>> futures;
    threads.reserve(COUNT_THREADS);

    for (int i = 0; i < COUNT_THREADS; ++i) {
        std::promise<std::string> prom;
        futures.push_back(prom.get_future());    // future до move
        threads.emplace_back(funcThread,
                             std::cref(args[i]),
                             std::ref(logger),
                             std::move(prom));    // promise только move
    }

    // wait for stop all threads
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    // читаем результаты из futures (задание 14/16)
    for (auto& f : futures) {
        std::string result = f.get();
        logger.writeLine("main got: " + result);
    }

    // close file automatically
    logger.writeLine("main: all threads finished, file closed");

    // результат задания 20
    std::cout << "counter = " << counter
              << " (expected: " << (COUNT_THREADS * 100000) << ")\n";

    // ============ Задание 21. Производитель–потребитель ============
    {
        std::mutex              pc_mutex;
        std::condition_variable pc_cv;
        int                     pc_data  = 0;
        bool                    pc_ready = false;   // в буфере есть элемент
        bool                    pc_done  = false;   // producer закончил
        constexpr int           PC_COUNT = 10;

        // producer: кладёт значение в буфер
        auto producer = [&]() {
            for (int i = 1; i <= PC_COUNT; ++i) {
                std::unique_lock<std::mutex> lock(pc_mutex);
                // ждём, пока буфер пуст (ready == false)
                pc_cv.wait(lock, [&] { return !pc_ready; });

                pc_data  = i;
                pc_ready = true;

                lock.unlock();
                pc_cv.notify_one();
            }

            // сообщаем о завершении
            std::lock_guard<std::mutex> lock(pc_mutex);
            pc_done = true;
            pc_cv.notify_one();
        };

        // consumer: забирает значение из буфера и пишет в лог
        auto consumer = [&]() {
            while (true) {
                std::unique_lock<std::mutex> lock(pc_mutex);
                // ждём, пока есть элемент ИЛИ producer закончил
                pc_cv.wait(lock, [&] { return pc_ready || pc_done; });

                if (pc_done && !pc_ready) {
                    break;   // producer закончил и буфер пуст
                }

                int value = pc_data;
                pc_ready  = false;

                lock.unlock();

                logger.writeLine("consumed: " + std::to_string(value));
                pc_cv.notify_one();
            }
        };

        std::thread t_prod(producer);
        std::thread t_cons(consumer);

        t_prod.join();
        t_cons.join();

        logger.writeLine("main: producer-consumer finished");
    }
    // ================================================================

    return 0;
}
