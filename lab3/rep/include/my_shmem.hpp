#pragma once

#include <string.h>   // strlen()
#include <stdlib.h>   // malloc()
#include <stdio.h>    // sprintf
#include <new>        // placement new

#if defined (WIN32)
#   include <windows.h>
#	define MAP_NAME_PREFIX "Local\\"
#	define INV_HANDLE (NULL)
#	define CSEM        HANDLE
#else
#   include <sys/mman.h>
#   include <sys/stat.h>        
#   include <fcntl.h>           
#   include <unistd.h>          
#   include <semaphore.h>       
#   define HANDLE          int
#   define INV_HANDLE      (-1)
#	define MAP_NAME_PREFIX  "/"
#	define CSEM            sem_t*
#endif

#define SEM_NAME_POSTFIX "_sem"

namespace cplib
{
    template <class T> class SharedMem
    {
    public:
        SharedMem(const char* name, bool create_if_not_exists = true) :_fd(INV_HANDLE), _mem(NULL), _sem(NULL) {
            // Формируем имена с учетом префиксов платформы
            _fname = (char*)malloc(strlen(name) + strlen(MAP_NAME_PREFIX) + 1);
            if(strlen(MAP_NAME_PREFIX) > 0) memcpy(_fname, MAP_NAME_PREFIX, strlen(MAP_NAME_PREFIX));
            strcpy(_fname + strlen(MAP_NAME_PREFIX), name);
            
            _semname = (char*)malloc(strlen(_fname) + strlen(SEM_NAME_POSTFIX) + 1);
            strcpy(_semname, _fname);
            strcat(_semname, SEM_NAME_POSTFIX);

            bool is_new = false;
            // 1. Пытаемся открыть существующую
            bool ret = OpenMem(_fname, _semname);
            
            // 2. Если не вышло и флаг create_if_not_exists=true — создаем новую
            if (!ret && create_if_not_exists) {
                if ((ret = CreateMem(_fname, _semname)))
                    is_new = true;
            }
            
            // 3. Отображаем память в адресное пространство
            if (ret)
                ret = MapMem();

            // 4. Инициализация (только для создателя)
            if (ret && is_new) {
                // Обнуляем память
                memset(_mem, 0, sizeof(shmem_contents));
                // Вызываем конструктор T "по месту"
                new (&_mem->data) T(); 
                _mem->ref_cnt = 0;
            }

            if (ret) {
                // Увеличиваем счетчик подключенных процессов (атомарно, под защитой)
                Lock();
                _mem->ref_cnt++;
                Unlock();
            }
            else {
                // Если сбой, чистим за собой
                if (is_new) DestroyMem();
                else CloseMem();
            }
        }

        virtual ~SharedMem() {
            if (IsValid()) {
                Lock();
                _mem->ref_cnt--;
                int cnt = _mem->ref_cnt;
                Unlock();
                
                CloseMem();
                
                // Если мы последние — удаляем объект из системы
                if (cnt <= 0) {
                     DestroyMem();
                }
            }
            free(_fname);
            free(_semname);
        }

        bool IsValid() { return _fd != INV_HANDLE && _mem != NULL; }
        
        // --- Блокировка (P-операция, wait) ---
        void Lock() {
#if defined (WIN32)
            WaitForSingleObject(_sem, INFINITE);
#else
            sem_wait(_sem);
#endif
        }

        // --- Разблокировка (V-операция, signal) ---
        void Unlock() {
#if defined (WIN32)
            ReleaseSemaphore(_sem, 1, NULL);
#else
            sem_post(_sem);
#endif
        }

        T* Data() {
            if (!IsValid()) return NULL;
            return &_mem->data;
        }

    private:
        bool OpenMem(const char* mem_name, const char* sem_name) {
#if defined (WIN32)
            _fd = OpenFileMappingA(FILE_MAP_WRITE, FALSE, mem_name);
            if (_fd != NULL)
                _sem = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, sem_name);
#else
            _fd = shm_open(mem_name, O_RDWR, 0644);
            if (_fd != INV_HANDLE) {
                _sem = sem_open(sem_name, 0);
                if (_sem == SEM_FAILED) {
                    _sem = NULL;
                    close(_fd);
                    _fd = INV_HANDLE;
                }
            }
#endif
            return (_fd != INV_HANDLE && _sem != NULL);
        }

        bool CreateMem(const char* mem_name, const char* sem_name) {
#if defined (WIN32)
            _fd = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(shmem_contents), mem_name);
            // Создаем семафор с начальным значением 1 (Свободен)
            if (_fd != NULL)
                _sem = CreateSemaphoreA(NULL, 1, 1, sem_name);
#else
            _fd = shm_open(mem_name, O_CREAT | O_RDWR, 0644);
            if (_fd != INV_HANDLE) {
                ftruncate(_fd, sizeof(shmem_contents));
                // Создаем семафор с начальным значением 1 (Свободен)
                _sem = sem_open(sem_name, O_CREAT, 0644, 1);
                if (_sem == SEM_FAILED) _sem = NULL;
            }
#endif
            return (_fd != INV_HANDLE && _sem != NULL);
        }

        bool MapMem() {
            if (_fd == INV_HANDLE) return false;
#if defined (WIN32)
            _mem = reinterpret_cast<shmem_contents*>(MapViewOfFile(_fd, FILE_MAP_WRITE, 0, 0, 0));
#else
            void* res = mmap(NULL, sizeof(shmem_contents), PROT_WRITE | PROT_READ, MAP_SHARED, _fd, 0);
            if (res == MAP_FAILED) _mem = NULL;
            else _mem = reinterpret_cast<shmem_contents*>(res);
#endif
            return (_mem != NULL);
        }

        void UnMapMem() {
            if (_mem == NULL) return;
#if defined (WIN32)
            UnmapViewOfFile(_mem);
#else
            munmap(_mem, sizeof(shmem_contents));
#endif
            _mem = NULL;
        }

        void CloseMem() {
            UnMapMem();
            if (_fd != INV_HANDLE) {
#if defined (WIN32)
                CloseHandle(_fd);
#else
                close(_fd);
#endif      
                _fd = INV_HANDLE;
            }
            if (_sem != NULL) {
#if defined (WIN32)
                CloseHandle(_sem);
#else
                sem_close(_sem);
#endif
                _sem = NULL;
            }
        }

        void DestroyMem() {
#if !defined (WIN32)
            shm_unlink(_fname);
            sem_unlink(_semname);
#endif
        }

        // Внутренняя структура: счетчик ссылок + данные пользователя
        struct shmem_contents {
            int ref_cnt; 
            T   data;    
        } *_mem;

        CSEM   _sem;
        HANDLE _fd;
        char* _fname;
        char* _semname;
    };
}
