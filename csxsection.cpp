#include "csxsection.h"
#include <algorithm>
#include <cwctype>

CSXSection::CSXSection(Stream* stream)
{
    stream->read(&m_header, sizeof(section_header_t));
    m_data.resize(m_header.size);
    stream->read(&m_data[0], m_header.size);

    m_id = m_header.id;
    if (m_id.length() > 8)
        m_id.resize(8);
}

CSXSection::~CSXSection()
{
    m_data.resize(0);
}

void CSXSection::export_data_to_file(std::string name)
{
    Stream* str = new Stream(name, FILE_OPEN_WRITE_ST);
    str->write(&m_data[0], m_header.size);
    delete str;
}
