#include "csxutils.h"
#include <locale>
#include <codecvt>

CSXUtils::CSXUtils()
{

}

std::u16string CSXUtils::to_u16string(int const &i)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t, 0x10ffff, std::little_endian>, char16_t> conv;
    return conv.from_bytes(std::to_string(i));
}

std::u16string CSXUtils::read_unicode_string(Stream* str)
{
    /*char16_t buf[1024];
    uint32_t str_len;
    str->read(&str_len, sizeof(uint32_t));
    str->read(buf, str_len * sizeof(char16_t));
    buf[str_len] = u'\0';
    std::u16string s16 = buf;
    std::string res = conv.to_bytes(s16);
    return res;*/

    char16_t buf[1024];
    uint32_t str_len;
    str->read(&str_len, sizeof(uint32_t));
    str->read(buf, str_len * sizeof(char16_t));
    //for (uint32_t i=1 ; i<str_len ; ++i)
    //    buf[i] = buf[i*2];
    buf[str_len] = u'\0';
    return buf;
}
