#ifndef CSXSTRUCT_H
#define CSXSTRUCT_H

#include "stream.h"
#include <map>

#define FIELD_TYPE_STRUCT 0x00
#define FIELD_TYPE_REF 0x01
#define FIELD_TYPE_PARENT 0x02
#define FIELD_TYPE_UNK 0x03
#define FIELD_TYPE_INTEGER 0x04
#define FIELD_TYPE_REAL 0x05
#define FIELD_TYPE_STRING 0x06
#define FILED_TYPE_DEF 0x06 //TODO: name

enum class EVariableType
{
    STRUCT,
    REF,
    PARENT,
    UNK,
    INTEGER,
    REAL,
    STRING,
};

typedef struct variable_t
{
    EVariableType       type;
    std::u16string      name;
    std::u16string      struct_name;

    int32_t             value_integer;
    double              value_real;
    std::u16string      value_str;
} variable_t;

enum class EFunctionOperation
{
    INIT_LOCAL, // cat 0x00 op 0x01
    INIT_FIELD, // cat 0x00 op 0x02

    PUSH_VAL,   // cat 0x02 op 0x00
    PUSH_VAR,   // cat 0x02 op 0x01
    PUSH_FIELD, // cat 0x02 op 0x02 0x06
    PUSH_THIS,  // cat 0x02 op 0x02 0x01
    PUSH_OBJ,   // cat 0x02 op 0x03
    PUSH_UNK,   // cat 0x02 op 0x04

    POP_INC,    // cat 0x03 op 0x00
    POP_DEC,    // cat 0x03 op 0x01
    POP_MUL,    // cat 0x03 op 0x02
    POP_DIV,    // cat 0x03 op 0x03
    POP_MOD,    // cat 0x03 op 0x04
    POP_AND,    // cat 0x03 op 0x05
    POP_OR,     // cat 0x03 op 0x06
    POP_XOR,    // cat 0x03 op 0x07
    POP_VAR,    // cat 0x03 op 0x01

    JUMP,       // cat 0x06

    JUMP_COND,  // cat 0x07 op 0x00

    CALL_UNK,        // cat 0x08 op 0x00
    CALL_METHOD,     // cat 0x08 op 0x02
    CALL_FUNCTION,   // cat 0x08 op 0x05

    RET_NONE,   // cat 0x09 op 0x00
    RET_THIS,   // cat 0x09 op 0x01
    RET_UNK,    // cat 0x09 op 0x03

    GET_FIELD,  // cat 0x0a op 0x06

    FIELD_BY_OFFSET, // cat 0x0b

    MATH_ADD,   // cat 0x0c op 0x00
    MATH_SUB,   // cat 0x0c op 0x01
    MATH_MUL,   // cat 0x0c op 0x02
    MATH_DIV,   // cat 0x0c op 0x03
    MATH_MOD,   // cat 0x0c op 0x04
    MATH_AND,   // cat 0x0c op 0x05
    MATH_OR,    // cat 0x0c op 0x06
    MATH_XOR,   // cat 0x0c op 0x07
    MATH_UNK1,  // cat 0x0c op 0x08
    MATH_UNK2,  // cat 0x0c op 0x09

    LOGIC_UNK1, // cat 0x0d op 0x01
    LOGIC_UNK2, // cat 0x0d op 0x02
    LOGIC_UNK3, // cat 0x0d op 0x03

    CMP_NE,     // cat 0x0e op 0x00
    CMP_EQ,     // cat 0x0e op 0x01
    CMP_LE,     // cat 0x0e op 0x02
    CMP_LQ,     // cat 0x0e op 0x03
    CMP_GE,     // cat 0x0e op 0x04
    CMP_GQ,     // cat 0x0e op 0x05

    NOT_OPERATION,
    LABEL,
};

enum class EFunctionArgType
{
    IMMEDIATE_VALUE,
    FUNCTION_ADDR,
    FIELD_NAME,
};

typedef struct
{
    EFunctionOperation  op;

    variable_t          field_name;
} operation_t;

class CSXFunction
{
public:
    CSXFunction();
    ~CSXFunction();

    void decompile_from_bin(Stream* str);
    void listing_to_stream(Stream* str);

    std::u16string get_name() { return m_name; }

    uint32_t get_define_offset() { return m_define_offset; }

    std::u16string get_type_name(variable_t &var);
private:
    std::u16string    m_name;
    std::u16string    m_this;
    std::u16string    m_parent;
    uint32_t        m_define_offset;
    uint32_t        m_params_count;

    operation_t     *m_cur_op;

    std::vector<variable_t> m_variables;
    std::vector<operation_t*>m_operations;
    std::map<uint32_t,uint32_t>m_addr_to_op_idx;
    std::vector<uint32_t>   m_jumps_op_idx;
    std::vector<uint32_t>   m_labels;

    void parse_local_op(Stream* str);
    void parse_stack_op(Stream* str);
    void parse_pop_op(Stream* str);
    void parse_label_op(Stream* str);
    void parse_jump_op(Stream* str);
    void parse_jump_cond_op(Stream* str);
    void parse_call_op(Stream* str);
    void parse_ret_op(Stream* str);
    void parse_get_op(Stream* str);
    void parse_math_op(Stream* str);
    void parse_logic_op(Stream* str);
    void parse_cmd_op(Stream* str);

    variable_t parse_var_def(Stream* str);
    void parse_value(Stream* str);

    std::u16string var_to_string(variable_t *var);
    std::u16string oper_to_string(operation_t *oper, uint32_t op_idx);

    void throw_ex(std::string msg, uint32_t arg1, uint32_t arg2);
};

#endif // CSXSTRUCT_H
