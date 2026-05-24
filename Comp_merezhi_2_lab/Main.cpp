#include <windows.h>
#include <iostream>
#include <string>
#include <cstdio>

struct ThreadParams {
    int id;
    bool isPositive;
    int syncType; // 1: None, 2: Event, 3: CS
    HANDLE hHeap;
    HANDLE hGlobalSemaphore;
    HANDLE hEvent;
    CRITICAL_SECTION* pCS;
    int* pSharedData;
};

HANDLE hConsoleMutex;

static DWORD WINAPI ThreadProc(LPVOID lpParam) {
    ThreadParams* params = static_cast<ThreadParams*>(lpParam);
    if (!params) return 1;

    // Глобальний семафор обмежує кількість працюючих потоків до 2
    WaitForSingleObject(params->hGlobalSemaphore, INFINITE);

    char* buffer = static_cast<char*>(HeapAlloc(params->hHeap, HEAP_ZERO_MEMORY, 128));

    for (int i = 1; i <= 500; ++i) {
        int value = params->isPositive ? i : -i;

        // Синхронізація всередині пари
        if (params->syncType == 2) WaitForSingleObject(params->hEvent, INFINITE);
        else if (params->syncType == 3) EnterCriticalSection(params->pCS);

        // Робота зі спільною пам'яттю
        if (params->pSharedData) *(params->pSharedData) = value;

        // Рівний горизонтальний вивід
        WaitForSingleObject(hConsoleMutex, INFINITE);
        if ((i - 1) % 10 == 0) printf("\n[T%d]: ", params->id);
        printf("%5d ", value);
        ReleaseMutex(hConsoleMutex);

        if (params->syncType == 2) SetEvent(params->hEvent);
        else if (params->syncType == 3) LeaveCriticalSection(params->pCS);

        Sleep(5); // Пауза для наочності
    }

    HeapFree(params->hHeap, 0, buffer);
    ReleaseSemaphore(params->hGlobalSemaphore, 1, NULL);
    return 0;
}

int main() {
    SetConsoleCP(1251); SetConsoleOutputCP(1251);
    hConsoleMutex = CreateMutex(NULL, FALSE, NULL);

    HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), L"Local\\Lab2Mem");
    int* pSharedData = (int*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));

    // Об'єкти для синхронізації
    HANDLE hGlobalSemaphore = CreateSemaphore(NULL, 2, 2, NULL); // Дозволяє 2 потоки
    HANDLE hPairEvent = CreateEvent(NULL, FALSE, TRUE, NULL);     // Для пари 3-4
    CRITICAL_SECTION pairCS;
    InitializeCriticalSection(&pairCS);                         // Для пари 5-6

    HANDLE hThreads[6] = { 0 };
    ThreadParams tParams[6] = { 0 };

    for (int i = 0; i < 6; ++i) {
        int currentID = i + 1;
        tParams[i].id = currentID;

        // Логіка: 1, 3, 5 - додатні; 2, 4, 6 - від'ємні
        tParams[i].isPositive = (currentID % 2 != 0);

        // СУВОРИЙ розподіл типів за ID
        if (currentID <= 2) tParams[i].syncType = 1;      // T1, T2: No Sync
        else if (currentID <= 4) tParams[i].syncType = 2; // T3, T4: Event
        else tParams[i].syncType = 3;                     // T5, T6: CS

        tParams[i].hGlobalSemaphore = hGlobalSemaphore;
        tParams[i].hEvent = hPairEvent;
        tParams[i].pCS = &pairCS;
        tParams[i].pSharedData = pSharedData;
        tParams[i].hHeap = HeapCreate(0, 4096, 0);

        hThreads[i] = CreateThread(NULL, 0, ThreadProc, &tParams[i], CREATE_SUSPENDED, NULL);

        // Встановлюємо пріоритети (різні у парі)
        if (tParams[i].isPositive) SetThreadPriority(hThreads[i], THREAD_PRIORITY_HIGHEST);
        else SetThreadPriority(hThreads[i], THREAD_PRIORITY_LOWEST);
    }

    printf("Потоки готові (СУВОРИЙ ПОРЯДОК):\n");
    printf("T1(+), T2(-) -> No Sync\n");
    printf("T3(+), T4(-) -> Event\n");
    printf("T5(+), T6(-) -> Critical Section\n");
    printf("\nНатисніть клавішу для запуску...\n");
    system("pause");

    for (int i = 0; i < 6; ++i) ResumeThread(hThreads[i]);

    WaitForMultipleObjects(6, hThreads, TRUE, INFINITE);

    // Очищення
    for (int i = 0; i < 6; ++i) {
        CloseHandle(hThreads[i]);
        HeapDestroy(tParams[i].hHeap);
    }
    DeleteCriticalSection(&pairCS);
    CloseHandle(hPairEvent);
    CloseHandle(hGlobalSemaphore);
    CloseHandle(hConsoleMutex);
    UnmapViewOfFile(pSharedData);
    CloseHandle(hMapFile);

    printf("\n\nРоботу завершено.\n");
    system("pause");
    return 0;
}