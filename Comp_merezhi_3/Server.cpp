#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#define UDP_PORT 8888
#define TCP_PORT 9999

// Потік для обробки кожного підключеного клієнта
DWORD WINAPI ClientThread(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)lpParam;
    char buffer[1024];

    std::cout << "[TCP] Клієнт підключився.\n";

    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cout << "[TCP] Клієнт відключився.\n";
            break;
        }
        buffer[bytesReceived] = '\0';
        std::cout << "[ФОРУМ]: " << buffer << std::endl;

        // Тут можна додати розсилку повідомлення іншим клієнтам
    }

    closesocket(clientSocket);
    return 0;
}

// Потік для відповіді на UDP бродкаст (виявлення сервера)
DWORD WINAPI UdpDiscoveryThread(LPVOID lpParam) {
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    char buffer[1024];

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(UDP_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(udpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::cout << "[UDP] Сервер очікує бродкаст-запити для виявлення...\n";

    while (true) {
        int bytes = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrLen);
        if (bytes > 0) {
            std::cout << "[UDP] Запит від " << inet_ntoa(clientAddr.sin_addr) << ". Надсилаю підтвердження.\n";
            std::string msg = "SERVER_HERE";
            sendto(udpSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&clientAddr, clientAddrLen);
        }
    }
    return 0;
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // 1. Запуск потоку виявлення (UDP)
    CreateThread(NULL, 0, UdpDiscoveryThread, NULL, 0, NULL);

    // 2. Налаштування TCP Сервера
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in tcpAddr;
    tcpAddr.sin_family = AF_INET;
    tcpAddr.sin_port = htons(TCP_PORT);
    tcpAddr.sin_addr.s_addr = INADDR_ANY;

    bind(listenSocket, (sockaddr*)&tcpAddr, sizeof(tcpAddr));
    listen(listenSocket, SOMAXCONN);

    std::cout << "[TCP] Сервер чату чекає на TCP порту " << TCP_PORT << "...\n";

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            // Створюємо новий потік для кожного клієнта
            CreateThread(NULL, 0, ClientThread, (LPVOID)clientSocket, 0, NULL);
        }
    }

    WSACleanup();
    return 0;
}