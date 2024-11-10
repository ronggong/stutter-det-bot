#pragma once
#include <codecvt>
#include <locale>
#include <string>

std::string wcharToString(const wchar_t* wchar) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(wchar);
}