#ifndef CSXUTILS_H
#define CSXUTILS_H

#include "stream.h"

class CSXUtils
{
public:
    CSXUtils();

    static std::u16string to_u16string(int const &i);
    static std::u16string read_unicode_string(Stream* str);
};

#endif // CSXUTILS_H
