#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

// Структура для передачі даних через анонімні канали
struct PolynomialData {
    int degreeA;
    int degreeB;
    int coeffs[100]; // Масив для коефіцієнтів обох многочленів (A з 0, B з 50)
};

// Глобальні дескриптори
HANDLE hPipe1Read, hPipe1Write;
HANDLE hPipe2Read, hPipe2Write;
HANDLE hEventStage1Finished;
HANDLE hEventStage2Finished;
HANDLE hConsoleMutex;

// --- Потік 1: Ручне введення многочленів ---
DWORD WINAPI Stage1_InputGenerator(LPVOID lpParam) {
    // М'ютекс потрібен, щоб монополізувати консоль для введення
    WaitForSingleObject(hConsoleMutex, INFINITE);

    PolynomialData data = { 0 };

    std::cout << "=== ЕТАП 1: ВВЕДЕННЯ ДАНИХ ===\n";

    // Введення першого многочлена
    std::cout << "Введіть степінь многочлена A: ";
    std::cin >> data.degreeA;
    for (int i = 0; i <= data.degreeA; i++) {
        std::cout << "  Коефіцієнт a" << i << ": ";
        std::cin >> data.coeffs[i];
    }

    // Введення другого многочлена
    std::cout << "\nВведіть степінь многочлена B: ";
    std::cin >> data.degreeB;
    for (int i = 0; i <= data.degreeB; i++) {
        std::cout << "  Коефіцієнт b" << i << ": ";
        std::cin >> data.coeffs[50 + i]; // Записуємо зі зміщенням 50
    }

    std::cout << "[Потік 1]: Дані зібрано. Записую в Pipe 1...\n";
    DWORD written;
    WriteFile(hPipe1Write, &data, sizeof(data), &written, NULL);

    std::cout << "[Потік 1]: ФІНІШ.\n";
    std::cout << "-----------------------------------\n";

    ReleaseMutex(hConsoleMutex);

    // Повідомляємо Потоку 2, що можна починати
    SetEvent(hEventStage1Finished);
    return 0;
}

// --- Потік 2: Обчислення (Множення) ---
DWORD WINAPI Stage2_Calculator(LPVOID lpParam) {
    // Чекаємо фінішу Потоку 1
    WaitForSingleObject(hEventStage1Finished, INFINITE);

    PolynomialData input;
    int resultCoeffs[100] = { 0 };
    DWORD bytesRead, bytesWritten;

    ReadFile(hPipe1Read, &input, sizeof(input), &bytesRead, NULL);

    WaitForSingleObject(hConsoleMutex, INFINITE);
    std::cout << "[Потік 2]: Початок обчислення добутку...\n";

    // Алгоритм множення: C[i+j] += A[i] * B[j]
    for (int i = 0; i <= input.degreeA; ++i) {
        for (int j = 0; j <= input.degreeB; ++j) {
            resultCoeffs[i + j] += input.coeffs[i] * input.coeffs[50 + j];
        }
    }

    int resDegree = input.degreeA + input.degreeB;

    // Передаємо результат далі
    WriteFile(hPipe2Write, &resDegree, sizeof(int), &bytesWritten, NULL);
    WriteFile(hPipe2Write, resultCoeffs, sizeof(resultCoeffs), &bytesWritten, NULL);

    std::cout << "[Потік 2]: Розрахунок завершено. Результат у Pipe 2. ФІНІШ.\n";
    ReleaseMutex(hConsoleMutex);

    // Повідомляємо Потоку 3, що можна починати
    SetEvent(hEventStage2Finished);
    return 0;
}

// --- Потік 3: Вивід результату ---
DWORD WINAPI Stage3_Printer(LPVOID lpParam) {
    // Чекаємо фінішу Потоку 2
    WaitForSingleObject(hEventStage2Finished, INFINITE);

    int finalDegree;
    int finalCoeffs[100];
    DWORD bytesRead;

    ReadFile(hPipe2Read, &finalDegree, sizeof(int), &bytesRead, NULL);
    ReadFile(hPipe2Read, finalCoeffs, sizeof(finalCoeffs), &bytesRead, NULL);

    WaitForSingleObject(hConsoleMutex, INFINITE);
    std::cout << "\n=== ЕТАП 3: РЕЗУЛЬТАТ КОНВЕЄРА ===\n";
    std::cout << "P(x) = ";
    for (int i = 0; i <= finalDegree; i++) {
        if (finalCoeffs[i] == 0) continue;

        if (i > 0 && finalCoeffs[i] > 0) std::cout << " + ";

        std::cout << finalCoeffs[i];
        if (i > 0) std::cout << "x^" << i;
    }
    std::cout << "\n\n[Потік 3]: ФІНІШ.\n";
    ReleaseMutex(hConsoleMutex);

    return 0;
}

int main() {
    SetConsoleCP(1251); SetConsoleOutputCP(1251);

    hConsoleMutex = CreateMutex(NULL, FALSE, NULL);
    hEventStage1Finished = CreateEvent(NULL, TRUE, FALSE, NULL);
    hEventStage2Finished = CreateEvent(NULL, TRUE, FALSE, NULL);

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    CreatePipe(&hPipe1Read, &hPipe1Write, &sa, 0);
    CreatePipe(&hPipe2Read, &hPipe2Write, &sa, 0);

    HANDLE hThreads[3];
    hThreads[0] = CreateThread(NULL, 0, Stage1_InputGenerator, NULL, 0, NULL);
    hThreads[1] = CreateThread(NULL, 0, Stage2_Calculator, NULL, 0, NULL);
    hThreads[2] = CreateThread(NULL, 0, Stage3_Printer, NULL, 0, NULL);

    WaitForMultipleObjects(3, hThreads, TRUE, INFINITE);

    for (int i = 0; i < 3; i++) CloseHandle(hThreads[i]);
    CloseHandle(hPipe1Read); CloseHandle(hPipe1Write);
    CloseHandle(hPipe2Read); CloseHandle(hPipe2Write);
    CloseHandle(hEventStage1Finished); CloseHandle(hEventStage2Finished);
    CloseHandle(hConsoleMutex);

    std::cout << "Програма завершена. Натисніть клавішу...\n";
    system("pause");
    return 0;
}