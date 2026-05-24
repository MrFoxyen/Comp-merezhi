#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

int main() {
    SetConsoleCP(1251); SetConsoleOutputCP(1251);

    HANDLE hMutex = CreateMutex(NULL, FALSE, L"Global\\SaturationMutex");
    if (!hMutex) return 1;

    // ВАЖЛИВО: Назва твого .exe файлу
    std::wstring workerPath = L"WorkerProject.exe";
    std::vector<HANDLE> hProcs;

    std::cout << "--- ЛР №5: Starvation (Нескінченне відтермінування) ---\n";
    std::cout << "Запуск 10-ти Агресорів. Всі мають ОДНАКОВИЙ пріоритет.\n";

    for (int i = 0; i < 10; i++) {
        std::wstring cmd = workerPath + L" Aggressor " + std::to_wstring(i + 1);
        STARTUPINFO si = { sizeof(si) }; PROCESS_INFORMATION pi = { 0 };
        if (CreateProcess(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            hProcs.push_back(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    std::cout << "Агресори почали циклічну роботу. Запускаю Жертву...\n";
    Sleep(2000);

    std::wstring cmdV = workerPath + L" Victim 1";
    STARTUPINFO siV = { sizeof(siV) }; PROCESS_INFORMATION piV = { 0 };
    if (CreateProcess(NULL, (LPWSTR)cmdV.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &siV, &piV)) {
        hProcs.push_back(piV.hProcess);
        CloseHandle(piV.hThread);
    }

    // Чекаємо завершення лише Жертви (вона остання в списку)
    WaitForSingleObject(hProcs.back(), INFINITE);

    std::cout << "\nДемонстрацію завершено. Закриваю агресорів...\n";
    for (HANDLE h : hProcs) {
        TerminateProcess(h, 0);
        CloseHandle(h);
    }
    CloseHandle(hMutex);

    system("pause");
    return 0;
}