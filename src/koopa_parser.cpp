#include <cassert>
#include <iostream>
#include <string>
#include "koopa.h"
#include "koopa_parser.h"

#define Indentation (std::string(indent_level, ' '))
#define OUT (o << Indentation)

// 程序
void KoopaParser::Visit(const koopa_raw_program_t &program) {
    OUT << "Global values:" << std::endl;
    indent_level += 2;
    Visit(program.values);
    indent_level -= 2;
    OUT << "Functions:" << std::endl;
    indent_level += 2;
    Visit(program.funcs);
    indent_level -= 2;
    OUT << std::endl;
}

// 组成程序的各种类型的数据组, 如函数, 基本块, 指令等
void KoopaParser::Visit(const koopa_raw_slice_t &slice) {
    /* koopa raw 类型为其数据类型的指针
       根据 slice.kind 转化并调用 Visit */
    for (int i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                // 函数
                Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                // 基本块
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
                // 指令
                Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            default:
                // 暂时不会遇到其他内容, 于是不对其做任何处理
                assert(false);
        }
    }
}

// 函数
void KoopaParser::Visit(const koopa_raw_function_t &func) {
    // 访问所有基本块
    Visit(func->bbs);
}

// 基本块
void KoopaParser::Visit(const koopa_raw_basic_block_t &bb) {
    // 访问所有指令
    Visit(bb->insts);
}

// 指令
void KoopaParser::Visit(const koopa_raw_value_t &value) {
    // 根据指令类型判断后续需要如何访问
    auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_RETURN:
            // 返回指令
            OUT << "Return" << std::endl;
            if (kind.data.ret.value) {
                indent_level += 2;
                Visit(kind.data.ret.value);
                indent_level -= 2;
            }
            break;
        case KOOPA_RVT_INTEGER:
            // 整数指令
            OUT << "Integer " << kind.data.integer.value << std::endl;
            break;
        default:
            // 其他类型暂时遇不到
            assert(false);
    }
}
