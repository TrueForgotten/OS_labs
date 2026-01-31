#include <iostream>
#include <vector>
#include <string>
#include "background_launcher.hpp"

#ifdef _WIN32

#include <iostream>

using namespace std;

ProcessHandle BackgroundLauncher::launch(const vector<string> &args)
{
    if (args.empty()) {
        throw runtime_error("Нет аргументов");
    }

    //Командная строка для Windows.
    string command_line;
    for (const auto &arg : args)
    {
        if (!command_line.empty())
            command_line += " ";
        command_line += arg; 
    }

    vector<char> cmd(command_line.begin(), command_line.end());
    cmd.push_back(0); 

    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(
            nullptr,                
            cmd.data(),     
            nullptr,                
            nullptr,                
            FALSE,                  
            0,                      
            nullptr,                
            nullptr,                
            &si,                    
            &pi))                   
    {
        throw runtime_error("Не получилось запустить CreateProcess :(");
    }

    CloseHandle(pi.hThread);

    return pi.hProcess;
}

bool BackgroundLauncher::wait(ProcessHandle handle, int *exit_code)
{
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

    CloseHandle(handle);
    return true;
}

#else

#include <unistd.h>
#include <sys/wait.h>

using namespace std;

ProcessHandle BackgroundLauncher::launch(const vector<string> &args)
{
    if (args.empty()) {
        throw runtime_error("Нет аргументов");
    }

    //Вектор аргументов для Linux.
    vector<char *> argv;
    for (const auto &arg : args)
    {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr); // Обязательный NULL в конце

    // Клонируем 
    pid_t pid = fork(); 

    if (pid == -1)
        throw runtime_error("Не получилось запустить fork :(");

    if (pid == 0)
    {
        // Работа дочернего процесса
        execvp(argv[0], argv.data());

        perror("execvp failed");
        exit(EXIT_FAILURE); 
    }

    // Работа родителя
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
        // Проверяем, завершился ли процесс нормально
        if (WIFEXITED(status)) 
            *exit_code = WEXITSTATUS(status); 
        else
            return false; // Процесс упал или был убит
    }
    return true;
}

#endif