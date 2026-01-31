#pragma once

#include <string>

// Структура данных
struct SharedContext {
    int counter;
    int master_pid;      // int часто удобнее для pid_t в кроссплатформе
    bool copy1_active;
    bool copy2_active;
    long long last_master_beat;

    SharedContext() : counter(0), master_pid(0), copy1_active(false), copy2_active(false), last_master_beat(0) {}
};

// --- Прототипы функций (без реализации) ---

// Получить ID текущего процесса
int get_pid();

// Получить текущее время строкой
std::string get_current_time_str();

// Запустить копию процесса
void spawn_process(const std::string& arg);

// Записать сообщение в лог
void log_to_file(const std::string& filename, const std::string& msg);