#pragma once
#include "pch.h"
#define CURL_STATICLIB
#include <curl.h>
class API
{
public:
    static size_t CallBack(void*, size_t, size_t, std::string*);
    static void SendGet(const std::string&);
    static std::string GameServer(const std::string&, const std::string&, int);
    static std::string GetResponse(const std::string&);
};