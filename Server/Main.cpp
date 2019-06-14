#include <iostream>
#include "Server.h"

int main()
{
    Server::Start([](const std::string_view requestBody)
        {
            return std::string(requestBody);
        });
}