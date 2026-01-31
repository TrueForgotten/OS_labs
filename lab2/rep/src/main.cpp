#include <iostream>
#include <vector>
#include <string>
#include "background_launcher.hpp"

// --- Кроссплатформенный сон ---
#ifdef _WIN32
    #include <windows.h>
    // В Windows функция Sleep принимает миллисекунды
    #define MY_SLEEP_SEC(s) Sleep((s) * 1000)
#else
    #include <thread>
    #include <chrono>
    // В Linux используем стандартный C++ сон
    #define MY_SLEEP_SEC(s) std::this_thread::sleep_for(std::chrono::seconds(s))
#endif

using namespace std;

int main() {
    // Настройка кодировки для Windows (чтобы русский текст был читаем)
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    cout << "=== Тест фонового режима ===" << endl;

    vector<string> args;

#ifdef _WIN32
    // Windows: пингуем 3 раза
    args = {"ping", "127.0.0.1", "-n", "3"};
#else
    // POSIX: пингуем 3 раза
    args = {"ping", "127.0.0.1", "-c", "3"};
#endif

    cout << "[Родитель]: Запускаю дочерний процесс (Ping)..." << endl;

    try {
        // 1. ЗАПУСК
        ProcessHandle proc = BackgroundLauncher::launch(args);
        
        // 2. ДОКАЗАТЕЛЬСТВО ФОНОВОЙ РАБОТЫ
        cout << "[Родитель]: Процесс запущен! PID/Handle получен." << endl;
        cout << "[Родитель]: Сейчас я буду делать свою работу параллельно с пингом." << endl;

        for (int i = 1; i <= 3; ++i) {
            // Используем наш надежный макрос для сна
            MY_SLEEP_SEC(1); 
            cout << "[Родитель]: Я работаю... (шаг " << i << "/3)" << endl;
        }

        cout << "[Родитель]: Моя работа закончена. Жду завершения дочернего процесса..." << endl;

        // 3. ОЖИДАНИЕ
        int exitCode = 0;
        if (BackgroundLauncher::wait(proc, &exitCode)) {
            cout << "[Родитель]: Дочерний процесс завершился. Код возврата: " << exitCode << endl;
        } else {
            cerr << "[Родитель]: Ошибка при ожидании процесса." << endl;
        }

    } catch (const exception& e) {
        cerr << "[Ошибка]: " << e.what() << endl;
        return 1;
    }

    return 0;
}