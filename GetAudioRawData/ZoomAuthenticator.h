#pragma once

#undef min
#undef max
#include <string>

#include <jwt-cpp/jwt.h>


using namespace jwt;
using namespace std;
class ZoomAuthenticator
{
public:
    static wstring getJwt(const string& sdk_key, const string& sdk_secret);

};