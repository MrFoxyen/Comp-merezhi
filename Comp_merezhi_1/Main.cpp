#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Функція для отримання повного шляху до поточного .exe файлу
string GetCurrentExePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return string(buffer);
}

// Функція для запуску дочірнього процесу
void StartChild(string args, bool inheritHandles) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Використовуємо шлях до нашого ж EXE + аргументи
    string fullCmd = "\"" + GetCurrentExePath() + "\" " + args;
    char* cmdLine = _strdup(fullCmd.c_str()); // CreateProcess потребує не-const рядок

    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, inheritHandles, 0, NULL, NULL, &si, &pi)) {
        cout << "[Error] CreateProcess failed! Code: " << GetLastError() << endl;
    }
    else {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    free(cmdLine);
}

int main(int argc, char* argv[]) {
    // 1. ПЕРЕВІРКА НА ОДИН ЗАПУЩЕНИЙ ЕКЗЕМПЛЯР (через іменований м'ютекс)
    HANDLE hNamedMutex = CreateMutexA(NULL, TRUE, "Global\\MyUniqueLab1Mutex_CompMerezhi");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (argc == 1) {
            cout << "[System] Another instance is already running. Exiting..." << endl;
            return 0;
        }
    }

    // ЛОГІКА ДОЧІРНЬОГО ПРОЦЕСУ-РОБОЧОГО
    if (argc > 1 && string(argv[1]) == "worker") {
        int id = stoi(argv[2]);
        HANDLE hSemaphore = (HANDLE)stoll(argv[3]);

        WaitForSingleObject(hSemaphore, INFINITE);
        cout << "Process #" << id << " is WORKING (ID: " << GetCurrentProcessId() << ")" << endl;
        Sleep(2000);
        cout << "Process #" << id << " FINISHED." << endl;
        ReleaseSemaphore(hSemaphore, 1, NULL);
        return 0;
    }

    // ЛОГІКА ДОЧІРНЬОГО ПРОЦЕСУ-СПАДКОЄМЦЯ
    if (argc > 1 && string(argv[1]) == "descendant") {
        HANDLE hUnnamedMutex = (HANDLE)stoll(argv[2]);
        cout << "[Descendant] Waiting for unnamed mutex..." << endl;
        WaitForSingleObject(hUnnamedMutex, INFINITE);
        cout << "[Descendant] Received mutex! Work started..." << endl;
        Sleep(1500);
        ReleaseMutex(hUnnamedMutex);
        cout << "[Descendant] Released mutex and exiting." << endl;
        return 0;
    }

    // ГОЛОВНИЙ ПРОЦЕС
    cout << "=== Main Process Started (PID: " << GetCurrentProcessId() << ") ===" << endl;

    // 2. Неіменований м'ютекс для спадковості
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE; // ДОЗВОЛЯЄМО СПАДКУВАННЯ
    HANDLE hUnnamedMutex = CreateMutexA(&sa, FALSE, NULL);

    cout << "[Parent] Launching descendant with inherited mutex..." << endl;
    StartChild("descendant " + to_string((long long)hUnnamedMutex), true);

    // 3. Семафор для 10 процесів (макс. 3 одночасно)
    HANDLE hSemaphore = CreateSemaphoreA(&sa, 3, 3, NULL);
    vector<HANDLE> processes;

    cout << "[Parent] Launching 10 worker processes (max 3 at once)..." << endl;
    for (int i = 1; i <= 10; i++) {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        string cmd = "\"" + GetCurrentExePath() + "\" worker " + to_string(i) + " " + to_string((long long)hSemaphore);
        char* cmdBuf = _strdup(cmd.c_str());

        if (CreateProcessA(NULL, cmdBuf, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            processes.push_back(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        free(cmdBuf);
    }

    // 4. ТАЙМЕР НА 5 СЕКУНД
    HANDLE hTimer = CreateWaitableTimerA(NULL, TRUE, NULL);
    LARGE_INTEGER liDueTime;
    liDueTime.QuadPart = -50000000LL; // 5 секунд
    SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);

    cout << "[Parent] Timer set for 5 seconds. Waiting..." << endl;
    WaitForSingleObject(hTimer, INFINITE);
    cout << "[Parent] TIMER SIGNALED! Checking child processes status:" << endl;

    // 5. ПЕРЕВІРКА СТАНУ ЧЕРЕЗ WaitForSingleObject (timeout 0)
    int finishedCount = 0;
    for (int i = 0; i < processes.size(); i++) {
        DWORD res = WaitForSingleObject(processes[i], 0);
        if (res == WAIT_OBJECT_0) {
            cout << " - Worker " << i + 1 << ": COMPLETED" << endl;
            finishedCount++;
        }
        else {
            cout << " - Worker " << i + 1 << ": STILL RUNNING" << endl;
        }
        CloseHandle(processes[i]);
    }

    cout << "\nSummary: " << finishedCount << " / 10 processes finished within 5s." << endl;

    // Очищення
    CloseHandle(hUnnamedMutex);
    CloseHandle(hSemaphore);
    CloseHandle(hTimer);
    CloseHandle(hNamedMutex);

    cout << "\nPress Enter to exit parent process..." << endl;
    cin.get();
    return 0;
}