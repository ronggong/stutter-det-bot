#include "ZoomAuthenticator.h"
#include <chrono>
#include <codecvt>
#include <locale>


wstring ZoomAuthenticator::getJwt(const string& sdk_key, const string& sdk_secret)
{

    const auto iat = std::chrono::system_clock::now();
    const auto exp = iat + std::chrono::hours{ 24 };

    const auto token = jwt::create()
        .set_type("JWT")
        .set_issued_at(iat)
        .set_expires_at(exp)
        .set_payload_claim("appKey", claim(sdk_key))
        .set_payload_claim("tokenExp", claim(exp))
        .sign(algorithm::hs256{ sdk_secret });

    wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;

    return converter.from_bytes(token);
}