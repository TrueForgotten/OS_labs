#include "background_launcher.hpp"
#include <stdexcept>
#include <vector>
#include <cstring>

// ==========================================
// Реализация для WINDOWS
// ==========================================
#ifdef _WIN32

#include <iostream>

using namespace std;

ProcessHandle BackgroundLauncher::launch(const vector<string> &args)
{
    if (args.empty()) {
        throw runtime_error("Arguments list is empty");
    }

    // 1. Формируем командную строку для Windows.
    // Windows требует одну длинную строку, а не массив.
    string command_line;
    for (const auto &arg : args)
    {
        if (!command_line.empty())
            command_line += " ";
        // Совет: В реальных проектах тут нужно экранировать кавычки,
        // если в пути есть пробелы. Но для лабы хватит и простой склейки.
        command_line += arg; 
    }

    // 2. Создаем изменяемый буфер.
    // CreateProcessA может модифицировать строку аргументов, поэтому 
    // передавать string.c_str() небезопасно. Делаем копию в vector<char>.
    vector<char> mutable_cmd(command_line.begin(), command_line.end());
    mutable_cmd.push_back(0); // Нуль-терминатор

    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // 3. Системный вызов запуска процесса
    if (!CreateProcessA(
            nullptr,                // Имя модуля (берем из командной строки)
            mutable_cmd.data(),     // Командная строка (изменяемая!)
            nullptr,                // Атрибуты безопасности процесса
            nullptr,                // Атрибуты безопасности потока
            FALSE,                  // Наследовать дескрипторы? Нет.
            0,                      // Флаги создания
            nullptr,                // Окружение (родительское)
            nullptr,                // Текущая директория (родительская)
            &si,                    // Информация о старте
            &pi))                   // Сюда вернется информация о процессе
    {
        throw runtime_error("CreateProcess failed");
    }

    // Важно: Закрываем дескриптор потока сразу, так как мы не будем управлять потоком.
    // Если этого не сделать — будет утечка ресурсов.
    CloseHandle(pi.hThread);

    // Возвращаем дескриптор процесса, чтобы можно было сделать wait
    return pi.hProcess;
}

bool BackgroundLauncher::wait(ProcessHandle handle, int *exit_code)
{
    // Ждем, пока объект ядра (процесс) перейдет в сигнальное состояние
    DWORD result = WaitForSingleObject(handle, INFINITE);
    
    if (result != WAIT_OBJECT_0)
        return false;

    if (exit_code)
    {
        DWORD code;
        if (!GetExitCodeProcess(handle, &code))
            return false;
        *exit_code = static_cast<int>(code);
    }

    // После ожидания обязательно закрываем дескриптор процесса!
    CloseHandle(handle);
    return true;
}

// ==========================================
// Реализация для POSIX (Linux, macOS)
// ==========================================
#else

#include <unistd.h>
#include <sys/wait.h>

using namespace std;

ProcessHandle BackgroundLauncher::launch(const vector<string> &args)
{
    if (args.empty()) {
        throw runtime_error("Arguments list is empty");
    }

    // 1. Преобразуем vector<string> в массив char* для execvp
    // Формат: {"ls", "-l", NULL}
    vector<char *> argv;
    for (const auto &arg : args)
    {
        // const_cast нужен, так как execvp принимает char* (историческое наследие C)
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr); // Обязательный NULL в конце

    // 2. Клонируем текущий процесс
    pid_t pid = fork(); 

    if (pid == -1)
        throw runtime_error("fork failed");

    if (pid == 0)
    {
        // === МЫ В ДОЧЕРНЕМ ПРОЦЕССЕ ===
        // Заменяем текущий образ памяти новой программой.
        // execvp ищет программу в PATH.
        execvp(argv[0], argv.data());

        // Если мы дошли до этой строчки, значит execvp не сработал
        // (например, программа не найдена).
        perror("execvp failed");
        exit(EXIT_FAILURE); // Аварийно завершаем клона
    }

    // === МЫ В РОДИТЕЛЬСКОМ ПРОЦЕССЕ ===
    // Возвращаем PID ребенка пользователю
    return pid;
}

bool BackgroundLauncher::wait(ProcessHandle handle, int *exit_code)
{
    int status;
    // Ждем изменения состояния процесса с указанным PID
    if (waitpid(handle, &status, 0) == -1)
        return false;

    if (exit_code)
    {
        // Проверяем, завершился ли процесс нормально (не был убит сигналом)
        if (WIFEXITED(status)) 
            *exit_code = WEXITSTATUS(status); // Извлекаем код возврата
        else
            return false; // Процесс упал или был убит
    }
    return true;
}

#endif
