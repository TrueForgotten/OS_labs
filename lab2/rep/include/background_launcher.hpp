#ifndef BACKGROUND_LAUNCHER_H
#define BACKGROUND_LAUNCHER_H

#include <string>
#include <vector>

// Определяем тип дескриптора процесса в зависимости от ОС
#ifdef _WIN32
    #include <windows.h>
    // В Windows работаем с HANDLE (указатель на ресурс ядра)
    using ProcessHandle = HANDLE;
#else
    #include <sys/types.h>
    // В POSIX работаем с PID (целое число)
    using ProcessHandle = pid_t;
#endif

/// @brief Класс для кроссплатформенного запуска процессов
class BackgroundLauncher
{
public:
    /**
     * @brief Запускает программу в фоновом режиме.
     * * @param args Вектор аргументов. args[0] - имя программы.
     * @return ProcessHandle Дескриптор запущенного процесса (PID или HANDLE).
     * @throws std::runtime_error Если не удалось создать процесс.
     */
    static ProcessHandle launch(const std::vector<std::string> &args);

    /**
     * @brief Ожидает завершения процесса.
     * * @param handle Дескриптор процесса.
     * @param exit_code Указатель на переменную для кода возврата (опционально).
     * @return true Если процесс успешно завершился и код получен.
     * @return false Если произошла ошибка ожидания.
     */
    static bool wait(ProcessHandle handle, int *exit_code = nullptr);
};

#endif // BACKGROUND_LAUNCHER_H
