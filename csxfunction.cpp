#include "csxfunction.h"
#include "csxutils.h"
#include "csximage.h"
#include "csxfile.h"
#include <locale>
#include <codecvt>
#include <algorithm>
#include <QString>

CSXFunction::CSXFunction()
{

}

CSXFunction::~CSXFunction()
{
    for (operation_t* oper : m_operations)
        delete oper;
}

#define OP_CAT_LOCAL 0x00
#define OP_CAT_NOP   0x01 //TODO
#define OP_CAT_STACK 0x02
#define OP_CAT_END_MATH 0x03
#define OP_CAT_LABEL 0x04
#define OP_CAT_NOP2 0x05 //TODO
#define OP_CAT_JUMP 0x06
#define OP_CAT_JUMP_COND  0x07
#define OP_CAT_CALL  0x08
#define OP_CAT_RET   0x09
#define OP_CAT_GET   0x0a
#define OP_CAT_FIELD_BY_OFFSET 0x0b
#define OP_CAT_MATH 0x0c
#define OP_CAT_LOGIC 0x0d
#define OP_CAT_CMP  0x0e

#define NAME_ON_TABLE 0x80000000

void CSXFunction::decompile_from_bin(Stream* str)
{
    m_define_offset = str->getPosition() - 1;
    m_name = CSXUtils::read_unicode_string(str);

    str->read(&m_params_count, sizeof(uint32_t));
    for (uint32_t i=0 ; i<m_params_count ; ++i)
        m_variables.push_back(parse_var_def(str));

    int op_idx = 0;
    bool loop_fl = true;
    while (loop_fl)
    {
        uint32_t offset = str->getPosition();

        if ((CSXFile::get_function_name(offset).length() > 0) || (str->atEnd()))
            break;

        m_addr_to_op_idx[offset] = op_idx;

        uint8_t op_cat;
        str->read(&op_cat, sizeof(uint8_t));
        m_cur_op = nullptr;
        switch (op_cat)
        {
        case OP_CAT_LOCAL:
            parse_local_op(str);
            break;
        case OP_CAT_NOP:
            {
                m_cur_op = new operation_t;
                m_cur_op->op = EFunctionOperation::NOT_OPERATION;
                m_operations.push_back(m_cur_op);
            }
            break;
        case OP_CAT_STACK:
            parse_stack_op(str);
            break;
        case OP_CAT_END_MATH:
            parse_pop_op(str);
            break;
        case OP_CAT_LABEL:
            parse_label_op(str);
            break;
        case OP_CAT_NOP2:
            {
                m_cur_op = new operation_t;
                m_cur_op->op = EFunctionOperation::NOT_OPERATION;
                m_operations.push_back(m_cur_op);
            }
            break;
        case OP_CAT_JUMP:
            parse_jump_op(str);
            m_jumps_op_idx.push_back(op_idx);
            break;
        case OP_CAT_JUMP_COND:
            parse_jump_cond_op(str);
            m_jumps_op_idx.push_back(op_idx);
            break;
        case OP_CAT_CALL:
            parse_call_op(str);
            break;
        case OP_CAT_RET:
            parse_ret_op(str);
            //loop_fl = false;
            break;
        case OP_CAT_GET:
            parse_get_op(str);
            break;
        case OP_CAT_FIELD_BY_OFFSET:
            {
                m_cur_op = new operation_t;
                m_cur_op->op = EFunctionOperation::FIELD_BY_OFFSET;
                m_operations.push_back(m_cur_op);
            }
            break;
        case OP_CAT_MATH:
            parse_math_op(str);
            break;
        case OP_CAT_LOGIC:
            parse_logic_op(str);
            break;
        case OP_CAT_CMP:
            parse_cmd_op(str);
            break;
        default:
            throw_ex("Unknown operation category: ", op_cat, offset);
        }
        //printf("%s\n", oper_to_string(m_cur_op, op_idx).c_str());
        ++op_idx;
    }

    for (uint32_t jump_op : m_jumps_op_idx)
    {
        operation_t* oper = m_operations[jump_op];
        int32_t offset = (int32_t)oper->field_name.value_integer;
        int32_t jump_size;
        if (oper->op == EFunctionOperation::JUMP)
            jump_size = 5;
        else
            jump_size = 6;

        for (auto kk : m_addr_to_op_idx)
            if (kk.second == jump_op)
            {
                jump_op = kk.first;
                break;
            }
        offset = jump_op + jump_size + offset;
        oper->field_name.value_integer = m_addr_to_op_idx[offset];
        m_labels.push_back(oper->field_name.value_integer);
    }
}

void CSXFunction::listing_to_stream(Stream* str)
{
    str->writeWideStr(L"Function " + QString::fromStdU16String(m_name).toStdWString() + L"(");

    if (m_params_count > 0)
        for (uint32_t i=0 ; i<m_params_count ; ++i)
        {
            if (m_variables[i].type == EVariableType::STRUCT)
                str->writeWideStr(QString::fromStdU16String(m_variables[i].struct_name).toStdWString());
            else
                str->writeWideStr(QString::fromStdU16String(get_type_name(m_variables[i])).toStdWString());
            str->writeWideStr(L" " + QString::fromStdU16String(m_variables[i].name).toStdWString());
            if (i<(m_params_count - 1))
                str->writeWideStr(L", ");
        }
    str->writeWideStr(L")\n");

    uint32_t op_idx = 0;
    for (operation_t* oper : m_operations)
    {
        str->writeWideStr(QString::fromStdU16String(oper_to_string(oper, op_idx++)).toStdWString() + L"\n");
    }

    str->writeWideStr(L"EndFunc\n\n");
}

std::u16string CSXFunction::get_type_name(variable_t &var)
{
    switch (var.type)
    {
    case EVariableType::STRUCT:
        return u"struct " + var.struct_name;
    case EVariableType::REF:
        return u"Reference";
    case EVariableType::PARENT:
        return u"parent";
    case EVariableType::UNK:
        return u"unknown_type";
    case EVariableType::INTEGER:
        return u"Integer";
    case EVariableType::REAL:
        return u"Real";
    case EVariableType::STRING:
        return u"String";
    default:
        return u"";
    }
}

#define OP_LOC_LOCAL_VAR  0x01
#define OP_LOC_INIT_FIELD 0x02

void CSXFunction::parse_local_op(Stream* str)
{
    uint32_t offset = str->getPosition();

    uint8_t op_code;
    m_cur_op = new operation_t;
    str->read(&op_code, sizeof(uint8_t));
    switch (op_code)
    {
    case OP_LOC_LOCAL_VAR:
        //m_cur_op->op = EFunctionOperation::INIT_LOCAL;
        //m_cur_op->field_name = parse_var_def(str);
        m_cur_op->op = EFunctionOperation::INIT_LOCAL;
        m_cur_op->field_name = parse_var_def(str);
        m_variables.push_back(m_cur_op->field_name);
        break;
    case OP_LOC_INIT_FIELD:
        m_cur_op->op = EFunctionOperation::INIT_FIELD;
        m_cur_op->field_name = parse_var_def(str);
        break;
    default:
        throw_ex("Unknown local operation: ", op_code, offset);
    }
    m_operations.push_back(m_cur_op);
}

#define OP_STACK_PUSH_VAL 0x00
#define OP_STACK_PUSH_VAR 0x01
#define OP_STACK_PUSH_THIS_FIELD 0x02
#define OP_STACK_PUSH_OBJ_LINK 0x03
#define OP_STACK_PUSH_UNK 0x04

#define OP_PUSH_THIS 0x01
#define OP_PUSH_FIELD 0x06

void CSXFunction::parse_stack_op(Stream* str)
{
    uint32_t offset = str->getPosition();

    uint8_t op_code;
    m_cur_op = new operation_t;
    str->read(&op_code, sizeof(uint8_t));
    switch (op_code)
    {
    case OP_STACK_PUSH_VAL:
        m_cur_op->op = EFunctionOperation::PUSH_VAL;
        parse_value(str);
        break;
    case OP_STACK_PUSH_VAR:
        {
            str->read(&op_code, sizeof(uint8_t));
            if (op_code != 0x04)
                throw_ex("Invalid variable location!", op_code, offset);
            m_cur_op->op = EFunctionOperation::PUSH_VAR;
            uint32_t var_idx;
            str->read(&var_idx, sizeof(uint32_t));
            m_cur_op->field_name.name = m_variables[var_idx].name;
        }
        break;
    case OP_STACK_PUSH_THIS_FIELD:
        str->read(&op_code, sizeof(uint8_t));
        switch (op_code)
        {
        case OP_PUSH_THIS:
            m_cur_op->op = EFunctionOperation::PUSH_THIS;
            break;
        case OP_PUSH_FIELD:
            m_cur_op->op = EFunctionOperation::PUSH_FIELD;
            m_cur_op->field_name.name = CSXUtils::read_unicode_string(str);
            break;
        default:
            throw_ex("Invalid variable location!", op_code, offset);
        }
        break;
    case OP_STACK_PUSH_OBJ_LINK:
        str->read(&op_code, sizeof(uint8_t));
        m_cur_op->op = EFunctionOperation::PUSH_OBJ;
        if (op_code == 0x04)
        {
            m_cur_op->field_name.name = CSXFile::get_linkinf(offset + 2);
            str->read(&m_cur_op->field_name.value_integer, sizeof(int32_t));
        }
        else if (op_code == 0x06)
            m_cur_op->field_name.name = CSXUtils::read_unicode_string(str);
        else
            throw_ex("Invalid object location!", op_code, offset);
        //str->seek(4, spCurrent); //TODO
        break;
    case OP_STACK_PUSH_UNK: //TODO
        {
            uint8_t unk1;
            uint32_t unk2;
            str->read(&unk1, sizeof(uint8_t));
            str->read(&m_cur_op->field_name.value_integer, sizeof(uint32_t));
            m_cur_op->op = EFunctionOperation::PUSH_UNK;
        }
        break;
    default:
        throw_ex("Unknown stack operation: ", op_code, offset);
    }
    m_operations.push_back(m_cur_op);
}

#define OP_POP_INC 0x00
#define OP_POP_DEC 0x01
#define OP_POP_MUL 0x02
#define OP_POP_DIV 0x03
#define OP_POP_MOD 0x04
#define OP_POP_AND 0x05
#define OP_POP_OR  0x06
#define OP_POP_XOR 0x07
#define OP_POP_TO_VAR 0xff

void CSXFunction::parse_pop_op(Stream* str)
{
    uint32_t offset = str->getPosition();

    uint8_t pop_code;
    m_cur_op = new operation_t;
    str->read(&pop_code, sizeof(uint8_t));
    switch (pop_code)
    {
    case OP_POP_INC:
        m_cur_op->op = EFunctionOperation::POP_INC;
        break;
    case OP_POP_DEC:
        m_cur_op->op = EFunctionOperation::POP_DEC;
        break;
    case OP_POP_MUL:
        m_cur_op->op = EFunctionOperation::POP_MUL;
        break;
    case OP_POP_DIV:
        m_cur_op->op = EFunctionOperation::POP_DIV;
        break;
    case OP_POP_MOD:
        m_cur_op->op = EFunctionOperation::POP_MOD;
        break;
    case OP_POP_AND:
        m_cur_op->op = EFunctionOperation::POP_AND;
        break;
    case OP_POP_OR:
        m_cur_op->op = EFunctionOperation::POP_OR;
        break;
    case OP_POP_XOR:
        m_cur_op->op = EFunctionOperation::POP_XOR;
        break;
    case OP_POP_TO_VAR:
        m_cur_op->op = EFunctionOperation::POP_VAR;
        break;
    default:
        throw_ex("Unknown pop operation: ", pop_code, offset);
        break;
    }

    m_operations.push_back(m_cur_op);
}

void CSXFunction::parse_label_op(Stream* str)
{
    //uint32_t offset = str->getPosition();

    m_cur_op = new operation_t;
    m_cur_op->op = EFunctionOperation::LABEL;

    m_cur_op->field_name.name = CSXUtils::read_unicode_string(str);

    uint32_t unk1;
    str->read(&unk1, sizeof(uint32_t));

    if (m_cur_op->field_name.name.compare(u"@TRY") == 0)
    {
        // read offset if catch
        uint8_t unk2;
        str->read(&unk2, sizeof(uint8_t));
        str->read(&m_cur_op->field_name.value_integer, sizeof(uint32_t));
    }

    m_operations.push_back(m_cur_op);
}

void CSXFunction::parse_jump_op(Stream* str)
{
    m_cur_op = new operation_t;
    int32_t jump_off;
    str->read(&jump_off, sizeof(int32_t));
    m_cur_op->op = EFunctionOperation::JUMP;
    m_cur_op->field_name.value_integer = jump_off;
    m_operations.push_back(m_cur_op);
}

#define OP_JUMP_RELATIVE 0x00 //TODO

void CSXFunction::parse_jump_cond_op(Stream* str)
{
    uint32_t offset = str->getPosition();

    m_cur_op = new operation_t;

    uint8_t jump_type;
    str->read(&jump_type, sizeof(uint8_t));
    int32_t jump_off;
    str->read(&jump_off, sizeof(int32_t));
    switch (jump_type)
    {
    case OP_JUMP_RELATIVE:
        m_cur_op->op = EFunctionOperation::JUMP_COND;
        m_cur_op->field_name.value_integer = jump_off;
        break;
    default:
        throw_ex("Unknown jump operation: ", jump_type, offset);
    }

    m_operations.push_back(m_cur_op);
}

#define OP_CALL_UNK 0x00 //TODO
#define OP_CALL_METHOD 0x02
#define OP_CALL_FUNCTION 0x05

void CSXFunction::parse_call_op(Stream* str)
{
    uint32_t offset = str->getPosition();

    m_cur_op = new operation_t;

    uint8_t call_type;
    str->read(&call_type, sizeof(uint8_t));
    switch (call_type)
    {
    case OP_CALL_UNK:
        m_cur_op->op = EFunctionOperation::CALL_UNK;
        break;
    case OP_CALL_METHOD:
        m_cur_op->op = EFunctionOperation::CALL_METHOD;
        break;
    case OP_CALL_FUNCTION:
        m_cur_op->op = EFunctionOperation::CALL_FUNCTION;
        break;
    default:
        throw_ex("Unknown call operation: ", call_type, offset);
    }

    uint32_t code;
    str->read(&code, sizeof(uint32_t));
    //if (code != 0x1)
    //    throw_ex("Unknown call operation: ", code, offset);

    uint32_t is_on_table;
    str->read(&is_on_table, sizeof(uint32_t));
    if (is_on_table == NAME_ON_TABLE)
    {
        uint32_t name_offset = str->getPosition();
        str->read(&is_on_table, sizeof(uint32_t));

        m_cur_op->field_name.name = CSXFile::get_conststr(name_offset);
    }
    else
    {
        str->seek(str->getPosition() - 4, spBegin);
        m_cur_op->field_name.name = CSXUtils::read_unicode_string(str);
    }

    m_operations.push_back(m_cur_op);
}

#define OP_RET_NONE 0x00
#define OP_RET_THIS 0x01
#define OP_RET_UNK 0x03 //TODO

void CSXFunction::parse_ret_op(Stream* str)
{
    uint32_t offset = str->getPosition();

    uint8_t ret_type;
    m_cur_op = new operation_t;
    str->read(&ret_type, sizeof(uint8_t));
    switch (ret_type)
    {
    case OP_RET_NONE:
        m_cur_op->op = EFunctionOperation::RET_NONE;
        break;
    case OP_RET_THIS:
        m_cur_op->op = EFunctionOperation::RET_THIS;
        break;
    case OP_RET_UNK:
        m_cur_op->op = EFunctionOperation::RET_UNK;
        break;
    default:
        throw_ex("Unknown return operation: ", ret_type, offset);
    }
    m_operations.push_back(m_cur_op);
}

#define OP_GET_FIELD 0x06

void CSXFunction::parse_get_op(Stream* str)
{
    uint32_t offset = str->getPosition();

    uint8_t get_type;
    m_cur_op = new operation_t;
    str->read(&get_type, sizeof(uint8_t));

    switch (get_type)
    {
    case OP_GET_FIELD:
        m_cur_op->op = EFunctionOperation::GET_FIELD;
        break;
    default:
        throw_ex("Unknown get operation: ", get_type, offset);
    }

    uint32_t is_on_table;
    str->read(&is_on_table, sizeof(uint32_t));
    if (is_on_table == NAME_ON_TABLE)
    {
        uint32_t name_offset = str->getPosition();
        str->read(&is_on_table, sizeof(uint32_t));

        m_cur_op->field_name.name = CSXFile::get_conststr(name_offset);
    }
    else
    {
        str->seek(str->getPosition() - 4, spBegin);
        m_cur_op->field_name.name = CSXUtils::read_unicode_string(str);
    }

    m_operations.push_back(m_cur_op);
}

#define OP_MATH_ADD 0x00
#define OP_MATH_SUB 0x01
#define OP_MATH_MUL 0x02
#define OP_MATH_DIV 0x03
#define OP_MATH_MOD 0x04
#define OP_MATH_AND 0x05
#define OP_MATH_OR  0x06
#define OP_MATH_XOR 0x07
#define OP_MATH_UNK1 0x08 //TODO
#define OP_MATH_UNK2 0x09 //TODO

void CSXFunction::parse_math_op(Stream* str)
{
    uint32_t offset = str->getPosition();

    uint8_t math_type;
    m_cur_op = new operation_t;
    str->read(&math_type, sizeof(uint8_t));
    switch (math_type)
    {
    case OP_MATH_ADD:
        m_cur_op->op = EFunctionOperation::MATH_ADD;
        break;
    case OP_MATH_SUB:
        m_cur_op->op = EFunctionOperation::MATH_SUB;
        break;
    case OP_MATH_MUL:
        m_cur_op->op = EFunctionOperation::MATH_MUL;
        break;
    case OP_MATH_DIV:
        m_cur_op->op = EFunctionOperation::MATH_DIV;
        break;
    case OP_MATH_MOD:
        m_cur_op->op = EFunctionOperation::MATH_MOD;
        break;
    case OP_MATH_AND:
        m_cur_op->op = EFunctionOperation::MATH_AND;
        break;
    case OP_MATH_OR:
        m_cur_op->op = EFunctionOperation::MATH_OR;
        break;
    case OP_MATH_XOR:
        m_cur_op->op = EFunctionOperation::MATH_XOR;
        break;
    case OP_MATH_UNK1:
        m_cur_op->op = EFunctionOperation::MATH_UNK1;
        break;
    case OP_MATH_UNK2:
        m_cur_op->op = EFunctionOperation::MATH_UNK2;
        break;
    default:
        throw_ex("Unknown math operation: ", math_type, offset);
    }
    m_operations.push_back(m_cur_op);
}

#define OP_LOGIC_UNK1 0x01
#define OP_LOGIC_UNK2 0x02
#define OP_LOGIC_UNK3 0x03

void CSXFunction::parse_logic_op(Stream* str)
{
    uint32_t offset = str->getPosition();

    uint8_t lg_type;
    m_cur_op = new operation_t;
    str->read(&lg_type, sizeof(uint8_t));
    switch (lg_type)
    {
    case OP_LOGIC_UNK1:
        m_cur_op->op = EFunctionOperation::LOGIC_UNK1;
        break;
    case OP_LOGIC_UNK2:
        m_cur_op->op = EFunctionOperation::LOGIC_UNK2;
        break;
    case OP_LOGIC_UNK3:
        m_cur_op->op = EFunctionOperation::LOGIC_UNK3;
        break;
    default:
        throw_ex("Unknown logic operation: ", lg_type, offset);
    }
    m_operations.push_back(m_cur_op);
}

#define OP_CMP_NE 0x00
#define OP_CMP_EQ 0x01
#define OP_CMP_LE 0x02
#define OP_CMP_LQ 0x03
#define OP_CMP_GE 0x04
#define OP_CMP_GQ 0x05

void CSXFunction::parse_cmd_op(Stream* str)
{
    uint32_t offset = str->getPosition();

    uint8_t cmp_type;
    m_cur_op = new operation_t;
    str->read(&cmp_type, sizeof(uint8_t));
    switch (cmp_type)
    {
    case OP_CMP_NE:
        m_cur_op->op = EFunctionOperation::CMP_NE;
        break;
    case OP_CMP_EQ:
        m_cur_op->op = EFunctionOperation::CMP_EQ;
        break;
    case OP_CMP_LE:
        m_cur_op->op = EFunctionOperation::CMP_LE;
        break;
    case OP_CMP_LQ:
        m_cur_op->op = EFunctionOperation::CMP_LQ;
        break;
    case OP_CMP_GE:
        m_cur_op->op = EFunctionOperation::CMP_GE;
        break;
    case OP_CMP_GQ:
        m_cur_op->op = EFunctionOperation::CMP_GQ;
        break;
    default:
        throw_ex("Unknown comparasion operation: ", cmp_type, offset);
    }
    m_operations.push_back(m_cur_op);
}

variable_t CSXFunction::parse_var_def(Stream* str)
{
    uint32_t start_offset = str->getPosition();

    uint8_t var_type;
    str->read(&var_type, sizeof(uint8_t));
    variable_t var;
    switch (var_type)
    {
    case FIELD_TYPE_STRUCT:
        {
            uint32_t is_on_table;
            str->read(&is_on_table, sizeof(uint32_t));
            if (is_on_table == NAME_ON_TABLE)
            {
                uint32_t name_offset = str->getPosition();
                str->read(&is_on_table, sizeof(uint32_t));

                var.type = EVariableType::STRUCT;
                var.struct_name = CSXFile::get_conststr(name_offset);
            }
            else
            {
                str->seek(str->getPosition() - 4, spBegin);
                var.type = EVariableType::STRUCT;
                var.struct_name = CSXUtils::read_unicode_string(str);
            }
            var.name = CSXUtils::read_unicode_string(str);
        }
        break;
    case FIELD_TYPE_REF:
        var.type = EVariableType::REF;
        var.name = CSXUtils::read_unicode_string(str);
        break;
    case FIELD_TYPE_INTEGER:
        var.type = EVariableType::INTEGER;
        var.name = CSXUtils::read_unicode_string(str);
        break;
    case FIELD_TYPE_REAL:
        var.type = EVariableType::REAL;
        var.name = CSXUtils::read_unicode_string(str);
        break;
    case FIELD_TYPE_STRING:
        var.type = EVariableType::STRING;
        var.name = CSXUtils::read_unicode_string(str);
        break;
    case FIELD_TYPE_PARENT:
        var.type = EVariableType::PARENT;
        var.name = CSXUtils::read_unicode_string(str);
        break;
    case FIELD_TYPE_UNK:
        var.type = EVariableType::UNK;
        var.name = CSXUtils::read_unicode_string(str);
        break;
    default:
        throw_ex("Unknown variable type: ", var_type, start_offset);
        break;
    }
    return var;
}

void CSXFunction::parse_value(Stream* str)
{
    uint32_t start_offset = str->getPosition();

    uint8_t val_type;
    str->read(&val_type, sizeof(uint8_t));
    switch (val_type)
    {
    case FIELD_TYPE_STRUCT:
        {
            uint32_t is_on_table;
            str->read(&is_on_table, sizeof(uint32_t));
            if (is_on_table == NAME_ON_TABLE)
            {
                uint32_t name_offset = str->getPosition();
                str->read(&is_on_table, sizeof(uint32_t));

                m_cur_op->field_name.type = EVariableType::STRUCT;
                m_cur_op->field_name.struct_name = CSXFile::get_conststr(name_offset);
            }
            else
            {
                str->seek(str->getPosition() - 4, spBegin);
                m_cur_op->field_name.type = EVariableType::STRUCT;
                m_cur_op->field_name.struct_name = CSXUtils::read_unicode_string(str);
            }
        }
        break;
    case FIELD_TYPE_REF:
        m_cur_op->field_name.type = EVariableType::REF;
        break;
    case FIELD_TYPE_INTEGER:
        m_cur_op->field_name.type = EVariableType::INTEGER;
        str->read(&m_cur_op->field_name.value_integer, sizeof(uint32_t));
        break;
    case FIELD_TYPE_REAL:
        m_cur_op->field_name.type = EVariableType::REAL;
        str->read(&m_cur_op->field_name.value_real, sizeof(double));
        break;
    case FIELD_TYPE_STRING:
        m_cur_op->field_name.type = EVariableType::STRING;
        m_cur_op->field_name.value_str = CSXUtils::read_unicode_string(str);
        break;
    default:
        throw_ex("Unknown value type: ", val_type, start_offset);
        break;
    }
}

std::u16string CSXFunction::var_to_string(variable_t *var)
{
    switch (var->type)
    {
    case EVariableType::STRUCT:
        return u"struct";
    case EVariableType::REF:
        return u"ref";
    case EVariableType::PARENT:
        return u"parent";
    case EVariableType::INTEGER:
        return CSXUtils::to_u16string(var->value_integer) + u"i";
    case EVariableType::REAL:
        return CSXUtils::to_u16string(var->value_real) + u"d";
    case EVariableType::STRING:
        return u"\"" + var->value_str + u"\"";
    default:
        return u"";
    }
}

std::u16string CSXFunction::oper_to_string(operation_t *oper, uint32_t op_idx)
{
    if (oper == nullptr)
        return u"";

    std::u16string res = u"";

    if(std::find(m_labels.begin(), m_labels.end(), op_idx) != m_labels.end())
        res += u"@_l_" + CSXUtils::to_u16string(op_idx) + u":\n";

    switch (oper->op)
    {
    case EFunctionOperation::INIT_LOCAL:
        res += u"\tINIT_LOCAL " + get_type_name(oper->field_name) + u" " + oper->field_name.name;
        break;
    case EFunctionOperation::INIT_FIELD:
        res += u"\tINIT_FIELD " + get_type_name(oper->field_name) + u" " + oper->field_name.name;
        break;
    case EFunctionOperation::PUSH_VAL:
        res += u"\tPUSH_VAL " + get_type_name(oper->field_name) + u" " + var_to_string(&oper->field_name);
        break;
    case EFunctionOperation::PUSH_VAR:
        res += u"\tPUSH_VAR " + oper->field_name.name;
        break;
    case EFunctionOperation::PUSH_FIELD:
        res += u"\tPUSH_FIELD " + oper->field_name.name;
        break;
    case EFunctionOperation::PUSH_THIS:
        res += u"\tPUSH_THIS " + oper->field_name.name;
        break;
    case EFunctionOperation::PUSH_OBJ:
        res += u"\tPUSH_OBJ " + oper->field_name.name + u" " + CSXUtils::to_u16string(oper->field_name.value_integer);
        break;
    case EFunctionOperation::PUSH_UNK:
        res += u"\tPUSH_UNK " + CSXUtils::to_u16string(oper->field_name.value_integer);
        break;
    case EFunctionOperation::POP_INC:
        res += u"\tPOP +=";
        break;
    case EFunctionOperation::POP_DEC:
        res += u"\tPOP -=";
        break;
    case EFunctionOperation::POP_MUL:
        res += u"\tPOP *=";
        break;
    case EFunctionOperation::POP_DIV:
        res += u"\tPOP /=";
        break;
    case EFunctionOperation::POP_MOD:
        res += u"\tPOP %=";
        break;
    case EFunctionOperation::POP_AND:
        res += u"\tPOP &=";
        break;
    case EFunctionOperation::POP_OR:
        res += u"\tPOP |=";
        break;
    case EFunctionOperation::POP_XOR:
        res += u"\tPOP ^=";
        break;
    case EFunctionOperation::POP_VAR:
        res += u"\tPOP =";
        break;
    case EFunctionOperation::JUMP:
        res += u"\tJUMP _l_" + CSXUtils::to_u16string(oper->field_name.value_integer);
        break;
    case EFunctionOperation::JUMP_COND:
        res += u"\tJUMP_OK _l_" + CSXUtils::to_u16string(oper->field_name.value_integer);
        break;
    case EFunctionOperation::CALL_UNK:
        res += u"\tCALL_UNK " + oper->field_name.name;
        break;
    case EFunctionOperation::CALL_METHOD:
        res += u"\tCALL_METHOD " + oper->field_name.name;
        break;
    case EFunctionOperation::CALL_FUNCTION:
        res += u"\tCALL " + oper->field_name.name;
        break;
    case EFunctionOperation::RET_NONE:
        res += u"\tReturn";
        break;
    case EFunctionOperation::RET_THIS:
        res += u"\tReturn this";
        break;
    case EFunctionOperation::RET_UNK:
        res += u"\tReturn unknown";
        break;
    case EFunctionOperation::GET_FIELD:
        res += u"\tGET_FIELD " + oper->field_name.name;
        break;
    case EFunctionOperation::FIELD_BY_OFFSET:
        res += u"\tGET_FIELD_BY_OFFSET";
        break;
    case EFunctionOperation::MATH_ADD:
        res += u"\t+";
        break;
    case EFunctionOperation::MATH_SUB:
        res += u"\t-";
        break;
    case EFunctionOperation::MATH_MUL:
        res += u"\t*";
        break;
    case EFunctionOperation::MATH_DIV:
        res += u"\t/";
        break;
    case EFunctionOperation::MATH_MOD:
        res += u"\t%";
        break;
    case EFunctionOperation::MATH_AND:
        res += u"\tand";
        break;
    case EFunctionOperation::MATH_OR:
        res += u"\tor";
        break;
    case EFunctionOperation::MATH_XOR:
        res += u"\txor";
        break;
    case EFunctionOperation::MATH_UNK1:
        res += u"\tmath_UNK2";
        break;
    case EFunctionOperation::MATH_UNK2:
        res += u"\tmath_UNK2";
        break;
    case EFunctionOperation::LOGIC_UNK1:
        res += u"\tlogic_unk1";
        break;
    case EFunctionOperation::LOGIC_UNK2:
        res += u"\tlogic_unk2";
        break;
    case EFunctionOperation::LOGIC_UNK3:
        res += u"\tlogic_unk3";
        break;
    case EFunctionOperation::CMP_NE:
        res += u"\t!=";
        break;
    case EFunctionOperation::CMP_EQ:
        res += u"\t==";
        break;
    case EFunctionOperation::CMP_LE:
        res += u"\t<=";
        break;
    case EFunctionOperation::CMP_LQ:
        res += u"\t<";
        break;
    case EFunctionOperation::CMP_GE:
        res += u"\t>";
        break;
    case EFunctionOperation::CMP_GQ:
        res += u"\t>=";
        break;
    case EFunctionOperation::NOT_OPERATION:
        res += u"\tNOP";
        break;
    case EFunctionOperation::LABEL:
        if (oper->field_name.name.length() > 0)
        {
            res += u"\t@" + oper->field_name.name;
            if (oper->field_name.name.compare(u"@TRY") == 0)
            {
                res += u" IF CATCH -> JUMP " + CSXUtils::to_u16string(oper->field_name.value_integer);
            }
            res += u"\n";
        }
        break;
    }
    return res;
}

void CSXFunction::throw_ex(std::string msg, uint32_t arg1, uint32_t arg2)
{
    printf("THROW: %s %X %X\n", msg.c_str(), arg1, arg2);
    exit(1);
}
