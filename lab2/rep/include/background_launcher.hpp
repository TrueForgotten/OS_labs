#include <string>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
    // HANDLE (указатель на ресурс ядра)
    using ProcessHandle = HANDLE;
#else
    #include <sys/types.h>
    // PID (целое число)
    using ProcessHandle = pid_t;
#endif

class BackgroundLauncher
{
public:

    static ProcessHandle launch(const std::vector<std::string> &args);

    static bool wait(ProcessHandle handle, int *exit_code = nullptr);
};
