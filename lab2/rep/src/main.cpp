#include <iostream>
#include <vector>
#include <string>
#include "background_launcher.hpp"

// У меня компилятор MinGW на винде не поддерживает sleep_for, поэтому придется делать так(
#ifdef _WIN32
    #include <windows.h>
    #define MY_SLEEP_SEC(s) Sleep((s) * 1000)
#else
    #include <thread>
    #include <chrono>
    #define MY_SLEEP_SEC(s) std::this_thread::sleep_for(std::chrono::seconds(s))
#endif

using namespace std;

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    vector<string> args;

// Выбрал пинг для запуска в фоне
#ifdef _WIN32
    args = {"ping", "127.0.0.1", "-n", "10"};
#else
    args = {"ping", "127.0.0.1", "-c", "10"};
#endif

    cout << "[Родитель]: Лаунч дочернего процесса.." << endl;

    try {

        ProcessHandle proc = BackgroundLauncher::launch(args);
        
        for (int i = 1; i <= 3; ++i) {
            MY_SLEEP_SEC(1); 
            cout << "[Родитель]: Работаем... (шаг " << i << "/3)" << endl;
        }

        cout << "[Родитель]: Ждем дочерний процесс..." << endl;

        int exitCode = 0;
        if (BackgroundLauncher::wait(proc, &exitCode)) {
            cout << "[Родитель]: Дочерний процесс завершился. Код возврата: " << exitCode << endl;
        } else {
            cerr << "[Родитель]: Что-то пошло не так.." << endl;
        }

    } catch (const exception& e) {
        cerr << "[Ошибка]: " << e.what() << endl;
        return 1;
    }

    return 0;
}