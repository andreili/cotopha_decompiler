#include "csximage.h"
#include <QString>

CSXImage* CSXImage::m_instance;

CSXImage::CSXImage()
{
    m_instance = this;
}

CSXImage::~CSXImage()
{
    for (auto func : m_functions)
        delete func;
}

#define PART_STRUCT_DEFINE 4

void CSXImage::decompile_bin(Stream* str, uint32_t pos)
{
    str->seek(pos, spBegin);
    uint8_t part_type;
    str->read(&part_type, sizeof(uint8_t));
    switch (part_type)
    {
    case PART_STRUCT_DEFINE:
        {
            CSXFunction* new_func = new CSXFunction();
            new_func->decompile_from_bin(str);
            m_functions.push_back(new_func);
        }
        break;
    default:
        printf("Unknown type! %x\n", part_type);
        break;
    }
}

void CSXImage::listing_to_dir(std::string dir_path)
{
    /*int idx = 0;
    for (CSXFunction* func : m_instance->m_functions)*/
    for (size_t idx=0 ; idx<m_instance->m_functions.size() ; ++idx)
    {
        CSXFunction* func = m_instance->m_functions[idx];
        //printf("%i/%i\n", ++idx, m_instance->m_functions.size());
        Stream *str = new Stream(dir_path + "/" +
                                 std::to_string(idx) +
                                 ".cos", FILE_OPEN_WRITE_ST);
        func->listing_to_stream(str);
        delete str;
    }
}

CSXFunction* CSXImage::get_function_by_offset(uint32_t offset)
{
    for (auto func : m_instance->m_functions)
        if (func->get_define_offset() == offset)
            return func;
    return nullptr;
}

