#include <iostream>
#include "Server.h"

#include <sstream>

std::string ExtractBody(const std::string_view s)
{
    const int fix = s.find("\n\r");
    if (fix == -1)
    {
        return {};
    }
    return std::string(s).substr(fix, 12);
}

int main()
{
    Server::Start([](const std::string_view req)
        {
            return ExtractBody(req);
        });
}