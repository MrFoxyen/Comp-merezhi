#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

constexpr int UDP_PORT = 8888;
constexpr int TCP_PORT = 9999;

// Потік для отримання повідомлень від сервера
DWORD WINAPI ReceiveMessagesThread(LPVOID lpParam) {
    SOCKET sock = (SOCKET)lpParam;
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break;
        buffer[bytesReceived] = '\0';

        // Виводимо повідомлення. \r повертає курсор на початок, щоб не псувати ввід користувача
        std::cout << "\r" << buffer << "\n> ";
    }
    return 0;
}

int main() {
    // Налаштування кодування
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // --- 0. ВВЕДЕННЯ НІКНЕЙМУ (ОБОВ'ЯЗКОВО) ---
    std::string nickname;
    std::cout << "===============================" << std::endl;
    std::cout << "ВВЕДІТЬ ВАШ НІКНЕЙМ: ";

    // std::ws ігнорує будь-які зайві символи переходу рядка на початку
    if (!(std::getline(std::cin >> std::ws, nickname))) {
        nickname = "User_" + std::to_string(GetCurrentProcessId());
    }

    // --- 1. ПОШУК СЕРВЕРА (UDP) ---
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    BOOL broadcast = TRUE;
    setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));

    int timeout = 3000;
    setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    sockaddr_in broadcastAddr = { 0 };
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(UDP_PORT);
    broadcastAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Пряме звернення для надійності на одному ПК

    std::cout << "[Система] Шукаю сервер..." << std::endl;
    std::string req = "FIND";
    sendto(udpSocket, req.c_str(), (int)req.length(), 0, (sockaddr*)&broadcastAddr, sizeof(broadcastAddr));

    sockaddr_in serverAddr = { 0 };
    int serverLen = sizeof(serverAddr);
    char buffer[1024];

    if (recvfrom(udpSocket, buffer, sizeof(buffer), 0, (sockaddr*)&serverAddr, &serverLen) > 0) {
        closesocket(udpSocket);

        // --- 2. ПІДКЛЮЧЕННЯ ТА ЧАТ (TCP) ---
        SOCKET tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        serverAddr.sin_port = htons(TCP_PORT);

        if (connect(tcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == 0) {
            std::cout << "[Система] Ви увійшли як [" << nickname << "]" << std::endl;
            std::cout << "-----------------------------------------------" << std::endl;

            // Запускаємо слухача повідомлень
            CreateThread(NULL, 0, ReceiveMessagesThread, (LPVOID)tcpSocket, 0, NULL);

            std::string text;
            while (true) {
                std::cout << "> ";
                std::getline(std::cin, text);

                if (text == "exit") break;
                if (text.empty()) continue;

                // Додаємо нік до повідомлення
                std::string fullMsg = "[" + nickname + "]: " + text;
                send(tcpSocket, fullMsg.c_str(), (int)fullMsg.length(), 0);
            }
        }
        closesocket(tcpSocket);
    }
    else {
        std::cout << "[Помилка] Сервер не знайдено за таймаутом." << std::endl;
    }

    WSACleanup();
    system("pause");
    return 0;
}
//синхронізувати їх по правильному так як записано на початку виводу.