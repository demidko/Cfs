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
    // Принимаем входящие соединения
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
        // ошибка получения данных
        closesocket(clientSocket);
        return Server::WhatHappened::RecvFailed;
    }
    if (result == 0)
    {
        // соединение закрыто клиентом
        return Server::WhatHappened::ConnectionClosed;
    }
    // result > 0 ?
    // Мы знаем фактический размер полученных данных, поэтому ставим метку конца строки
    // В буфере запроса.
    buf[result] = '\0';
    const auto answer = onRequest(buf);
    // Отправляем ответ клиенту с помощью функции send
    result = send(clientSocket, answer.c_str(), answer.length(), 0);
    // Закрываем соединение к клиентом
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
    WSADATA wsaData; // служебная структура для хранение информации
    // о реализации Windows Sockets
    // старт использования библиотеки сокетов процессом
    // (подгружается Ws2_32.dll)
    int cError = WSAStartup(MAKEWORD(2, 2), &wsaData);
    // Если произошла ошибка подгрузки библиотеки
    if (cError != 0)
    {
        return WhatHappened::LibraryLoadingError;
    }
    struct addrinfo *addr = NULL; // структура, хранящая информацию
    // об IP-адресе  слущающего сокета
    // Шаблон для инициализации структуры адреса
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // AF_INET определяет, что будет
    // использоваться сеть для работы с сокетом
    hints.ai_socktype = SOCK_STREAM; // Задаем потоковый тип сокета
    hints.ai_protocol = IPPROTO_TCP; // Используем протокол TCP
    hints.ai_flags = AI_PASSIVE; // Сокет будет биндиться на адрес,
    // чтобы принимать входящие соединения
    // Инициализируем структуру, хранящую адрес сокета - addr
    // Наш HTTP-сервер будет висеть на 8000-м порту локалхоста
    cError = getaddrinfo("127.0.0.1", "80", &hints, &addr);
    // Если инициализация структуры адреса завершилась с ошибкой,
    // выведем сообщением об этом и завершим выполнение программы
    if (cError != 0)
    {
        WSACleanup(); // выгрузка библиотеки Ws2_32.dll
        return WhatHappened::AddressInitializationError;
    }
    // Создание сокета
    int listenSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    // Если создание сокета завершилось с ошибкой, выводим сообщение,
    // освобождаем память, выделенную под структуру addr,
    // выгружаем dll-библиотеку и закрываем программу
    if (listenSocket == INVALID_SOCKET)
    {
        freeaddrinfo(addr);
        WSACleanup();
        return WhatHappened::SocketCreationError;
    }
    // Привязываем сокет к IP-адресу
    cError = bind(listenSocket, addr->ai_addr, (int)addr->ai_addrlen);
    // Если привязать адрес к сокету не удалось, то выводим сообщение
    // об ошибке, освобождаем память, выделенную под структуру addr.
    // и закрываем открытый сокет.
    // Выгружаем DLL-библиотеку из памяти и закрываем программу.
    if (cError == SOCKET_ERROR)
    {
        freeaddrinfo(addr);
        closesocket(listenSocket);
        WSACleanup();
        return WhatHappened::AddressBindingError;
    }
    // Инициализируем слушающий сокет
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
    // Убираем за собой
    closesocket(listenSocket);
    freeaddrinfo(addr);
    WSACleanup();
    return WhatHappened::Ok;
}

