#ifndef CSXIMAGE_H
#define CSXIMAGE_H

#include "csxfunction.h"

class CSXImage
{
public:
    CSXImage();
    ~CSXImage();

    void decompile_bin(Stream* str, uint32_t pos);
    void listing_to_dir(std::string dir_path);

    static CSXFunction* get_function_by_offset(uint32_t offset);

private:

private:
    static CSXImage         *m_instance;
    std::vector<CSXFunction*> m_functions;
};

#endif // CSXIMAGE_H
