#ifndef CSXSECTION_H
#define CSXSECTION_H

#include <inttypes.h>
#include "stream.h"

typedef struct
{
    char        id[8];
    uint32_t    size;
    uint32_t    unknown;
} section_header_t;

class CSXSection
{
public:
    CSXSection(Stream* stream);
    ~CSXSection();

    std::string get_id() { return m_id; }
    uint32_t get_size() { return m_header.size; }
    char* get_data() { return &m_data[0]; }

    void export_data_to_file(std::string name);
private:
    section_header_t    m_header;
    std::vector<char>   m_data;
    std::string         m_id;
};

#endif // CSXSECTION_H
