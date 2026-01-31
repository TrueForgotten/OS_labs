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

//Поток-счетчик
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

// Поток-мастер
class MasterThread : public Thread {
public:
    virtual void Main() {
        int ticks = 0;
        while (true) {
            Sleep(1.0);
            ticks++;

            // Логируем состояние
            g_mem->Lock();
            int val = g_mem->Data()->counter;
            bool busy1 = g_mem->Data()->copy1_active;
            bool busy2 = g_mem->Data()->copy2_active;

            g_mem->Data()->last_master_beat = time(NULL);
            g_mem->Unlock();

            FILE* f = fopen(LOG_FILE.c_str(), "a");
            if (f) {
                string time_str = get_current_time_str();
                fprintf(f, "%s | Поток-мастер (PID: %-5d) | Счетчик: %-10d\n", 
                        time_str.c_str(), get_pid(), val);
                fclose(f);
            }

            // Запуск копий раз в 3 секунды
            if (ticks % 3 == 0) {
                if (!busy1) spawn_process("copy1");
                if (!busy2) spawn_process("copy2");
            }
            CancelPoint();
        }
    }
};

// Поток-наблюдатель 
class MonitorThread : public Thread {
public:
    int my_pid;
    bool* is_master_ptr;         
    MasterThread* master_thread; 

    MonitorThread(int pid, bool* is_master, MasterThread* m_thr) 
        : my_pid(pid), is_master_ptr(is_master), master_thread(m_thr) {}

    virtual void Main() {
        while (true) {
            Sleep(1.0); // Проверяем раз в секунду

            // Проверка потока-мастера
            if (*is_master_ptr) {
                g_mem->Lock();
                // Подтверждаю, что я жив (Heartbeat)
                g_mem->Data()->last_master_beat = time(NULL);
                g_mem->Unlock();
            }
            // Проверка потока-подчиненного
            else {
                g_mem->Lock();
                long long now = time(NULL);
                long long last = g_mem->Data()->last_master_beat;
                
                // Поток мастер не отвечает более 5 секунд
                if (now - last > 5) {
                    cout << "\n>>> Теперь это - поток-мастер <<<" << endl;
                    cout << "> " << flush;
                    
                    // новый мастер
                    g_mem->Data()->master_pid = my_pid;
                    g_mem->Data()->last_master_beat = now;
                    
                    // меняем локальный статус
                    *is_master_ptr = true;
                    master_thread->Start(); 
                }
                g_mem->Unlock();
            }
            
            CancelPoint();
        }
    }
};

// Работа копий 
void work_as_copy1() {
    string t = get_current_time_str();
    char buf[256];
    sprintf(buf, "%s | Копия 1 СТАРТ       | PID: %-5d", t.c_str(), get_pid());
    log_to_file(LOG_FILE, string(buf));

    g_mem->Lock();
    if (!g_mem->Data()->copy1_active) {
        g_mem->Data()->copy1_active = true;
        g_mem->Data()->counter += 10;
        g_mem->Data()->copy1_active = false;
    }
    g_mem->Unlock();

    t = get_current_time_str();
    sprintf(buf, "%s | Копия 1 ВЫХОД       | PID: %-5d", t.c_str(), get_pid());
    log_to_file(LOG_FILE, string(buf));
}

void work_as_copy2() {
    string t = get_current_time_str();
    char buf[256];
    sprintf(buf, "%s | Копия 2 СТАРТ       | PID: %-5d", t.c_str(), get_pid());
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
    sprintf(buf, "%s | Копия 2 ВЫХОД       | PID: %-5d", t.c_str(), get_pid());
    log_to_file(LOG_FILE, string(buf));
}


int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // Инициализация памяти
    SharedMem<SharedContext> shmem("lab_os_memory");

    g_mem = &shmem;

    if (argc > 1) {
        string arg = argv[1];
        if (arg == "copy1") { 
            work_as_copy1();
             return 0; 
        }
        if (arg == "copy2") { 
            work_as_copy2(); 
            return 0; 
        }
    }

    // 3. Основной режим
    int pid = get_pid();
    string msg = get_current_time_str() + " | ЗАПУСК ПРОГРАММЫ    | PID: " + to_string(pid);
    log_to_file(LOG_FILE, msg);
    cout << msg << endl;

    bool is_master_process = false;
    
    // Запуск потока-мастера
    shmem.Lock();
    long long now = time(NULL);
    // Если мастера потока нет -  он мертв
    if (shmem.Data()->master_pid == 0 || (now - shmem.Data()->last_master_beat > 5)) {
        shmem.Data()->master_pid = pid;
        shmem.Data()->last_master_beat = now;
        is_master_process = true;
        cout << ">>> Я поток-мастер <<<" << endl;
    } else {
        cout << ">>> Я поток-подчиненный. Главный PID: " << shmem.Data()->master_pid << endl;
    }
    shmem.Unlock();

    // Создание потоков 
    
    MasterThread t_master;   
    CounterThread t_counter; 
    MonitorThread t_monitor(pid, &is_master_process, &t_master);  

    t_monitor.Start(); 
    t_counter.Start(); 

    if (is_master_process) {
        t_master.Start();
    }

    //Цикл работы
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

    // Завершение работы
    t_monitor.Stop();
    t_counter.Stop();
    
    if (is_master_process) {
        t_master.Stop();
    }

    if (is_master_process) {
        shmem.Lock();
        if (shmem.Data()->master_pid == pid) shmem.Data()->master_pid = 0;
        shmem.Unlock();
    }

    return 0;
}