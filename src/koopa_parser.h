#ifndef __KOOPA_PARSER_H__
#define __KOOPA_PARSER_H__

#include <iostream>
#include <unordered_map>
#include <string>
#include "koopa.h"

class KoopaParser {
private:
    int indent_level = 0;
    int stack_length = 0;
    std::unordered_map<koopa_raw_value_t, int> value_map;
    std::ostream &o;

public:
    KoopaParser(std::ostream &o) : o(o) {}
    ~KoopaParser() = default;

    void Visit(const koopa_raw_program_t &);
    void Visit(const koopa_raw_slice_t &);
    void Visit(const koopa_raw_function_t &);
    void Visit(const koopa_raw_basic_block_t &);
    void Visit(const koopa_raw_value_t &);

    int get_offset(const koopa_raw_value_t &);
    void set_offset(const koopa_raw_value_t &);
    void record_offset(const koopa_raw_value_t &);
    void record_offset(const koopa_raw_basic_block_t &);
    void record_offset(const koopa_raw_function_t &);

    void load_value(const koopa_raw_value_t &, const std::string&);
    void store_value(const koopa_raw_value_t &, const std::string&);

    void Compile(const koopa_raw_program_t &);
    void Compile(const koopa_raw_slice_t &);
    void Compile(const koopa_raw_function_t &);
    void Compile(const koopa_raw_basic_block_t &);
    void Compile(const koopa_raw_value_t &);

    void CompileReturn(const koopa_raw_value_t &);
    void CompileAlloc(const koopa_raw_value_t &);
    void CompileLoad(const koopa_raw_value_t &);
    void CompileStore(const koopa_raw_value_t &);
    void CompileBranch(const koopa_raw_value_t &);
    void CompileBinary(const koopa_raw_value_t &);
};

#endif
