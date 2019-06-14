#pragma comment(lib, "Ws2_32.lib")
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "Server.h"
#include <future>
#include <iostream>

static Server::WhatHappened Connection(const int listenSocket, Server::Handler onRequest)
{
    const int maxBufferSize = 20 * 1024;
    char buf[maxBufferSize];
    // ��������� �������� ����������
    int clientSocket = accept(listenSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET)
    {
        closesocket(listenSocket);
        WSACleanup();
        return Server::WhatHappened::RequestProcessingFatalError;
    }
    int result = recv(clientSocket, buf, maxBufferSize, 0);
    if (result == SOCKET_ERROR)
    {
        // ������ ��������� ������
        closesocket(clientSocket);
        return Server::WhatHappened::RecvFailed;
    }
    if (result == 0)
    {
        // ���������� ������� ��������
        return Server::WhatHappened::ConnectionClosed;
    }
    // result > 0 ?
    // �� ����� ����������� ������ ���������� ������, ������� ������ ����� ����� ������
    // � ������ �������.
    buf[result] = '\0';
    const auto answer = onRequest(buf);
    // ���������� ����� ������� � ������� ������� send
    result = send(clientSocket, answer.c_str(), answer.length(), 0);
    // ��������� ���������� � ��������
    closesocket(clientSocket);
    return result == SOCKET_ERROR ? Server::WhatHappened::SendFailed : Server::WhatHappened::Ok;
}

std::string Server::ToString(const WhatHappened what)
{
    using w = WhatHappened;
    switch (what)
    {
    case w::AddressBindingError: return "AddressBindingError";
    case w::AddressInitializationError: return "AddressInitializationError";
    case w::Ok: return "Ok";
    case w::LibraryLoadingError: return "LibraryLoadingError";
    case w::RequestHandlerIsNull: return "RequestHandlerIsNull";
    case w::RequestProcessingFatalError: return "RequestHandlerIsNull";
    case w::SocketCreationError: return "SocketCreationError";
    case w::SocketInitializationFailed: return "SocketCreationError";
    case w::RecvFailed: return "RecvFailed";
    case w::ConnectionClosed: return "ConnectionClosed";
    case w::SendFailed: "SendFailed";
    default: return "UnknownError";
    }
}

Server::WhatHappened Server::Start(Handler onRequest)
{
    if (onRequest == nullptr)
    {
        return WhatHappened::RequestHandlerIsNull;
    }
    WSADATA wsaData; // ��������� ��������� ��� �������� ����������
    // � ���������� Windows Sockets
    // ����� ������������� ���������� ������� ���������
    // (������������ Ws2_32.dll)
    int cError = WSAStartup(MAKEWORD(2, 2), &wsaData);
    // ���� ��������� ������ ��������� ����������
    if (cError != 0)
    {
        return WhatHappened::LibraryLoadingError;
    }
    struct addrinfo *addr = NULL; // ���������, �������� ����������
    // �� IP-������  ���������� ������
    // ������ ��� ������������� ��������� ������
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // AF_INET ����������, ��� �����
    // �������������� ���� ��� ������ � �������
    hints.ai_socktype = SOCK_STREAM; // ������ ��������� ��� ������
    hints.ai_protocol = IPPROTO_TCP; // ���������� �������� TCP
    hints.ai_flags = AI_PASSIVE; // ����� ����� ��������� �� �����,
    // ����� ��������� �������� ����������
    // �������������� ���������, �������� ����� ������ - addr
    // ��� HTTP-������ ����� ������ �� 8000-� ����� ����������
    cError = getaddrinfo("127.0.0.1", "80", &hints, &addr);
    // ���� ������������� ��������� ������ ����������� � �������,
    // ������� ���������� �� ���� � �������� ���������� ���������
    if (cError != 0)
    {
        WSACleanup(); // �������� ���������� Ws2_32.dll
        return WhatHappened::AddressInitializationError;
    }
    // �������� ������
    int listenSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    // ���� �������� ������ ����������� � �������, ������� ���������,
    // ����������� ������, ���������� ��� ��������� addr,
    // ��������� dll-���������� � ��������� ���������
    if (listenSocket == INVALID_SOCKET)
    {
        freeaddrinfo(addr);
        WSACleanup();
        return WhatHappened::SocketCreationError;
    }
    // ����������� ����� � IP-������
    cError = bind(listenSocket, addr->ai_addr, (int)addr->ai_addrlen);
    // ���� ��������� ����� � ������ �� �������, �� ������� ���������
    // �� ������, ����������� ������, ���������� ��� ��������� addr.
    // � ��������� �������� �����.
    // ��������� DLL-���������� �� ������ � ��������� ���������.
    if (cError == SOCKET_ERROR)
    {
        freeaddrinfo(addr);
        closesocket(listenSocket);
        WSACleanup();
        return WhatHappened::AddressBindingError;
    }
    // �������������� ��������� �����
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(listenSocket);
        WSACleanup();
        return WhatHappened::SocketInitializationFailed;
    }
    auto counter = 0ull;
    while (true)
    {
        std::vector<std::future<WhatHappened>> pool;
        for (int i = 0, threads = std::thread::hardware_concurrency() * 3; i < threads; ++i)
        {
            pool.push_back(std::async(std::launch::async, Connection, listenSocket, onRequest));
        }
        for (auto &i : pool)
        {
            const auto result = i.get();
            std::cout << ++counter << ": " << ToString(result) << std::endl;
            if (result == WhatHappened::RequestProcessingFatalError)
            {
                return WhatHappened::RequestProcessingFatalError;
            }
        }
    }
    // ������� �� �����
    closesocket(listenSocket);
    freeaddrinfo(addr);
    WSACleanup();
    return WhatHappened::Ok;
}

