#ifndef __KOOPA_PARSER_H__
#define __KOOPA_PARSER_H__

#include <iostream>
#include "koopa.h"

class KoopaParser {
private:
    int indent_level = 0;
    std::ostream &o;

public:
    KoopaParser(std::ostream &o) : o(o) {}
    ~KoopaParser() = default;

    void Visit(const koopa_raw_program_t &);
    void Visit(const koopa_raw_slice_t &);
    void Visit(const koopa_raw_function_t &);
    void Visit(const koopa_raw_basic_block_t &);
    void Visit(const koopa_raw_value_t &);

    void Compile(const koopa_raw_program_t &);
    void Compile(const koopa_raw_slice_t &);
    void Compile(const koopa_raw_function_t &);
    void Compile(const koopa_raw_basic_block_t &);
    void Compile(const koopa_raw_value_t &);
};

#endif
