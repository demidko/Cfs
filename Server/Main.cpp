#include <iostream>
#include "Server.h"
#include "json.hpp"
#include <sstream>

using nlohmann::json;

json JsonRequest(const std::string_view httpRequest)
{
    auto fix = httpRequest.find("\n\r");
    try
    {
        return (fix == -1 || (fix += 3) >= size(httpRequest)) ?
            json() : json::parse(httpRequest.substr(fix));
    }
    catch (const std::exception &e)
    {
        std::cout << "\n" << httpRequest << "\n\n" << e.what() << "\n" << std::endl;
        return {};
    }
}

std::string HttpResponse(const json &jsonObject)
{
    std::stringstream jsonStream;
    jsonStream << jsonObject;
    const auto body = jsonStream.str();
    std::stringstream response;
    response <<
        "HTTP/1.1 200 OK\r\n"
        "Version: HTTP/1.1\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: " << size(body)
        << "\r\n\r\n" << body;
    return response.str();
}

int main()
{
    std::cout << json(nullptr);
    /*Server::Start([](const std::string_view request)
        {
            const auto object = JsonRequest(request);
            
            
            const auto response = HttpResponse(
                { {"All", "IsOkey"} });
            return response;
        });*/
}