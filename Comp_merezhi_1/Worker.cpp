#include <windows.h>
#include <iostream>
#include <string>

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3) return 1;

    try {
        int id = std::stoi(argv[1]);
        // Отримуємо дескриптор успадкованого неіменованого м'ютекса
        HANDLE hInheritedMutex = (HANDLE)std::stoull(argv[2]);

        // Відкриваємо іменований семафор (створений головним процесом)
        HANDLE hSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"Global\\MySem9");

        if (hSemaphore && hInheritedMutex) {
            // 1. Чекаємо дозволу від семафора (макс. 3 процеси одночасно)
            WaitForSingleObject(hSemaphore, INFINITE);

            // 2. Використовуємо м'ютекс для входу в "критичну ділянку" виводу на екран
            WaitForSingleObject(hInheritedMutex, INFINITE);
            std::wcout << L"[Процес " << id << L"] почав роботу (PID: " << GetCurrentProcessId() << L")" << std::endl;
            ReleaseMutex(hInheritedMutex);

            // 3. Імітація роботи
            Sleep(2000);

            // 4. Знову м'ютекс для фінального виводу
            WaitForSingleObject(hInheritedMutex, INFINITE);
            std::wcout << L"[Процес " << id << L"] завершив роботу." << std::endl;
            ReleaseMutex(hInheritedMutex);

            // 5. Звільняємо місце для наступного процесу
            ReleaseSemaphore(hSemaphore, 1, NULL);

            // Закриваємо дескриптори
            CloseHandle(hSemaphore);
            CloseHandle(hInheritedMutex);
        }
    }
    catch (...) {
        return 1; // Якщо сталась помилка конвертації аргументів
    }

    return 0;
}