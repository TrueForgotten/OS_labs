#define _CRT_SECURE_NO_WARNINGS 
#include <iostream>
#include <string>
#include <vector>
#include <ctime> 

#include "my_thread.hpp"
#include "my_shmem.hpp"
#include "main_utils.hpp" 

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
using namespace cplib;

SharedMem<SharedContext>* g_mem = NULL; 
const string LOG_FILE = "log.txt";

// --- Поток Счетчик ---
class CounterThread : public Thread {
public:
    virtual void Main() {
        while (true) {
            Sleep(0.3);
            g_mem->Lock();
            g_mem->Data()->counter++;
            g_mem->Unlock();
            CancelPoint();
        }
    }
};

// --- Поток Мастер ---
class MasterThread : public Thread {
public:
    virtual void Main() {
        int ticks = 0;
        while (true) {
            Sleep(1.0);
            ticks++;

            g_mem->Lock();
            int val = g_mem->Data()->counter;
            bool busy1 = g_mem->Data()->copy1_active;
            bool busy2 = g_mem->Data()->copy2_active;
            // Теперь time() будет работать, так как мы подключили <ctime>
            g_mem->Data()->last_master_beat = time(NULL);
            g_mem->Unlock();

            FILE* f = fopen(LOG_FILE.c_str(), "a");
            if (f) {
                string time_str = get_current_time_str();
                fprintf(f, "%s | ГЛАВНЫЙ (PID: %-5d) | Счетчик: %-10d\n", 
                        time_str.c_str(), get_pid(), val);
                fclose(f);
            }

            if (ticks % 3 == 0) {
                if (!busy1) spawn_process("--copy1");
                if (!busy2) spawn_process("--copy2");
            }
            CancelPoint();
        }
    }
};

// --- Функции копий ---
void work_as_copy1() {
    string t = get_current_time_str();
    char buf[256];
    sprintf(buf, "%s | КОПИЯ 1 СТАРТ       | PID: %-5d", t.c_str(), get_pid());
    log_to_file(LOG_FILE, string(buf));

    g_mem->Lock();
    if (!g_mem->Data()->copy1_active) {
        g_mem->Data()->copy1_active = true;
        g_mem->Data()->counter += 10;
        g_mem->Data()->copy1_active = false;
    }
    g_mem->Unlock();

    t = get_current_time_str();
    sprintf(buf, "%s | КОПИЯ 1 ВЫХОД       | PID: %-5d", t.c_str(), get_pid());
    log_to_file(LOG_FILE, string(buf));
}

void work_as_copy2() {
    string t = get_current_time_str();
    char buf[256];
    sprintf(buf, "%s | КОПИЯ 2 СТАРТ       | PID: %-5d", t.c_str(), get_pid());
    log_to_file(LOG_FILE, string(buf));

    g_mem->Lock();
    if (g_mem->Data()->copy2_active) { 
        g_mem->Unlock(); return; 
    }
    g_mem->Data()->copy2_active = true;
    g_mem->Data()->counter *= 2;
    g_mem->Unlock();

    Thread::Sleep(2.0);

    g_mem->Lock();
    g_mem->Data()->counter /= 2;
    g_mem->Data()->copy2_active = false;
    g_mem->Unlock();

    t = get_current_time_str();
    sprintf(buf, "%s | КОПИЯ 2 ВЫХОД       | PID: %-5d", t.c_str(), get_pid());
    log_to_file(LOG_FILE, string(buf));
}

// --- MAIN ---
int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    SharedMem<SharedContext> shmem("lab_os_memory");
    if (!shmem.IsValid()) {
        printf("Ошибка: Не удалось открыть общую память!\n");
        return 1;
    }
    g_mem = &shmem;

    if (argc > 1) {
        string arg = argv[1];
        if (arg == "--copy1") { work_as_copy1(); return 0; }
        if (arg == "--copy2") { work_as_copy2(); return 0; }
    }

    int pid = get_pid();
    string msg = get_current_time_str() + " | ЗАПУСК ПРОГРАММЫ    | PID: " + to_string(pid);
    log_to_file(LOG_FILE, msg);
    cout << msg << endl;

    bool is_master_process = false;
    shmem.Lock();
    long long now = time(NULL); // Здесь тоже используется time()
    if (shmem.Data()->master_pid == 0 || (now - shmem.Data()->last_master_beat > 5)) {
        shmem.Data()->master_pid = pid;
        shmem.Data()->last_master_beat = now;
        is_master_process = true;
        cout << ">>> Я ГЛАВНЫЙ процесс (Master) <<<" << endl;
    } else {
        cout << ">>> Я ПОБОЧНЫЙ процесс. Главный PID: " << shmem.Data()->master_pid << endl;
    }
    shmem.Unlock();

    CounterThread t1;
    t1.Start();

    MasterThread t2;
    if (is_master_process) {
        t2.Start();
    }

    cout << "Введите число (изменить счетчик) или 'q' (выход):" << endl;
    char buf[100];
    while (true) {
        printf("> ");
        if (scanf("%s", buf) != 1) break;
        if (strcmp(buf, "q") == 0) break;

        int val = atoi(buf);
        if (val == 0 && buf[0] != '0') {
            printf("Ошибка, это не число\n");
            continue;
        }

        shmem.Lock();
        shmem.Data()->counter = val;
        shmem.Unlock();
        printf("Счетчик установлен в %d\n", val);
    }

    t1.Stop();
    if (is_master_process) {
        t2.Stop();
        shmem.Lock();
        if (shmem.Data()->master_pid == pid) shmem.Data()->master_pid = 0;
        shmem.Unlock();
    }

    return 0;
}