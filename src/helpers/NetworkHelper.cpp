#include "pch.h"
#include "NetworkHelper.h"

std::wstring GetLocalIPW()
{
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
    {
        return L"127.0.0.1";
    }

    struct addrinfo hints = {}, * res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0)
    {
        return L"127.0.0.1";
    }

    std::wstring ipResult = L"127.0.0.1";

    for (struct addrinfo* ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)ptr->ai_addr;

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);

        if (strncmp(ipStr, "127.", 4) != 0)
        {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, ipStr, -1, NULL, 0);
            std::wstring wstr(size_needed - 1, 0);
            MultiByteToWideChar(CP_UTF8, 0, ipStr, -1, &wstr[0], size_needed);

            ipResult = wstr;

            if (strncmp(ipStr, "192.168.", 8) == 0 || strncmp(ipStr, "10.", 3) == 0)
            {
                break;
            }
        }
    }

    if (res)
        freeaddrinfo(res);
    return ipResult;
}