#ifndef CSXFILE_H
#define CSXFILE_H

#include "csxsection.h"
#include "csximage.h"

typedef struct
{
    char        fileType[8];
    uint32_t    unknown1;
    uint32_t    zero;
    char        imageType[40];
    uint32_t    size;
    uint32_t    unknown2;
} CSXHeader_t;

typedef struct
{
    std::u16string  name;
    int32_t type;
    std::u16string  type_name;
} global_t;

#define offset_type std::pair<uint32_t,std::u16string>

class CSXFile
{
public:
    CSXFile();
    ~CSXFile();

    bool load_from_file(std::string file_name);

    void read_offsets();
    void print_offsets();

    void read_conststr();
    void print_conststr();

    void read_global();
    void print_global();

    void read_linkinf();

    void decompile();
    void listing_to_file(std::string fn);

    static std::u16string get_conststr(uint32_t offset) { return m_instance->m_conststr[offset]; }
    static std::u16string get_linkinf(uint32_t offset) { return m_instance->m_linkinf2[offset]; }
    static std::u16string get_function_name(uint32_t offset);

    void export_all_sections(std::string dir_name);
private:
    static CSXFile              *m_instance;
    CSXHeader_t                 m_header;
    std::vector<CSXSection*>    m_sections;
    CSXImage                    *m_image;

    //functions offsets
    std::vector<uint32_t>       m_init_offsets;
    std::vector<offset_type>    m_offsets;

    //constsrt
    std::map<uint32_t,std::u16string> m_conststr;

    //global
    std::vector<global_t>       m_global;

    //linkinf
    std::vector<uint32_t>       m_linkinf1;
    std::map<uint32_t,std::u16string> m_linkinf2;
};

#endif // CSXFILE_H
