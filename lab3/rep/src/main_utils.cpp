#include "main_utils.hpp"
#include <stdio.h>
#include <time.h>
#include <vector>

// Платформозависимые хедеры нужны только здесь, в реализации
#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <time.h>
#endif

int get_pid() {
#ifdef _WIN32
    return (int)GetCurrentProcessId();
#else
    return (int)getpid();
#endif
}

std::string get_current_time_str() {
    char buf[128];
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(buf, "%02d.%02d.%04d %02d:%02d:%02d.%03d", 
            st.wDay, st.wMonth, st.wYear,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm* tm_info = localtime(&ts.tv_sec);
    
    sprintf(buf, "%02d.%02d.%04d %02d:%02d:%02d.%03ld", 
            tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900,
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, ts.tv_nsec / 1000000);
#endif
    return std::string(buf);
}

void spawn_process(const std::string& arg) {
#ifdef _WIN32
    char filename[MAX_PATH];
    GetModuleFileNameA(NULL, filename, MAX_PATH);
    std::string cmd = std::string("\"") + filename + "\" " + arg;
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#else
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        char path[1024];
        ssize_t len = readlink("/proc/self/exe", path, sizeof(path)-1);
        if (len != -1) path[len] = '\0';
        else return; 

        execl(path, path, arg.c_str(), (char*)NULL);
        exit(1); 
    }
#endif
}

void log_to_file(const std::string& filename, const std::string& msg) {
    FILE* f = fopen(filename.c_str(), "a");
    if (f) {
        fprintf(f, "%s\n", msg.c_str());
        fclose(f);
    }
}