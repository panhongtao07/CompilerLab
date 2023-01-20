#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include "koopa.h"
#include "koopa_parser.h"

#define Indentation (std::string(indent_level, ' '))
#define OUT (o << Indentation)
#define real_name(v) (v->name + 1)
#define debug(v) (std::cout << v << std::endl)

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
        case KOOPA_RVT_BINARY:
            // 二元指令
            OUT << "Binary " << kind.data.binary.op << std::endl;
            indent_level += 2;
            Visit(kind.data.binary.lhs);
            Visit(kind.data.binary.rhs);
            indent_level -= 2;
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            // 全局变量分配指令
            OUT << "Global alloc " << real_name(value) << std::endl;
            break;
        default:
            // 其他类型暂时遇不到
            assert(false);
    }
}

// 编译器

// 寄存器和栈位置分配

// 获取指令在栈中的偏移量
int KoopaParser::get_offset(const koopa_raw_value_t &value) {
    std::cout << value->kind.tag << std::endl;
    assert(value_map.find(value) != value_map.end() && "value not found");
    return stack_length - value_map[value];
}

// 设置指令在栈中的偏移量
void KoopaParser::set_offset(const koopa_raw_value_t &value) {
    value_map[value] = stack_length;
    stack_length += 4;
}

// 遍历记录偏移量

// 指令
void KoopaParser::record_offset(const koopa_raw_value_t &value) {
    // 无论是否有返回值, 都记录4字节的偏移量即可
    set_offset(value);
}

// 基本块
void KoopaParser::record_offset(const koopa_raw_basic_block_t &bb) {
    // 访问所有指令
    auto &slice = bb->insts;
    assert(slice.kind == KOOPA_RSIK_VALUE);
    for (int i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        record_offset(reinterpret_cast<koopa_raw_value_t>(ptr));
    }
}

// 函数
void KoopaParser::record_offset(const koopa_raw_function_t &func) {
    // 访问所有基本块
    auto &slice = func->bbs;
    assert(slice.kind == KOOPA_RSIK_BASIC_BLOCK);
    for (int i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        record_offset(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
    }
}

// 加载值到寄存器
void KoopaParser::load_value(const koopa_raw_value_t &value, const std::string &reg) {
    // 根据指令类型判断后续需要如何访问
    auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_INTEGER:
            // 整数指令
            o << "li " << reg << ", " << kind.data.integer.value << std::endl;
            break;
        default:
            // 其他类型从栈中加载
            o << "lw " << reg << ", " << get_offset(value) << "(sp)" << std::endl;
    }
}

// 保存值到栈
void KoopaParser::store_value(const koopa_raw_value_t &value, const std::string &reg) {
    // 根据指令类型判断后续需要如何访问
    auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_INTEGER:
            // 整数指令
            assert(false);
            break;
        default:
            // 其他类型保存到栈中
            o << "sw " << reg << ", " << get_offset(value) << "(sp)" << std::endl;
    }
}

// 编译

// 程序
void KoopaParser::Compile(const koopa_raw_program_t &program) {
    // 函数
    if (program.funcs.len > 0) {
        o << ".text" << std::endl;
        Compile(program.funcs);
    }
    // 全局变量
    if (program.values.len > 0) {
        o << ".data" << std::endl;
        Compile(program.values);
    }
}

// 数据组

void KoopaParser::Compile(const koopa_raw_slice_t &slice) {
    for (int i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                // 函数
                Compile(reinterpret_cast<koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                // 基本块
                Compile(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
                // 指令
                Compile(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            default:
                // 暂时不会遇到其他内容, 于是不对其做任何处理
                assert(false);
        }
    }
}

// 函数
void KoopaParser::Compile(const koopa_raw_function_t &func) {
    // 声明函数
    o << ".globl " << real_name(func) << std::endl;
    // 标记入口
    o << real_name(func) << ":" << std::endl;
    // 遍历并记录栈偏移量
    stack_length = 0;
    record_offset(func);
    // 栈偏移量以16字节为单位向上取值
    int round_up = 16;
    stack_length = (stack_length + round_up - 1) / round_up * round_up;
    assert(stack_length < 0x800 && stack_length >= -0x800 && "stack overflow");
    // 保存栈帧
    o << "addi sp, sp, " << -stack_length << std::endl;
    // 编译所有基本块
    Compile(func->bbs);
    // 恢复栈帧由返回指令完成
}

// 基本块
void KoopaParser::Compile(const koopa_raw_basic_block_t &bb) {
    // 标记基本块入口
    o << real_name(bb) << ":" << std::endl;
    // 编译所有指令
    Compile(bb->insts);
}

// 指令
void KoopaParser::Compile(const koopa_raw_value_t &value) {
    // 根据指令类型判断后续需要如何访问
    auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_RETURN:
            // 返回指令
            CompileReturn(value);
            break;
        case KOOPA_RVT_INTEGER:
            // 整数指令
            o << kind.data.integer.value;
            break;
        case KOOPA_RVT_BINARY:
            // 二元指令
            CompileBinary(value);
            break;
        case KOOPA_RVT_ALLOC:
        case KOOPA_RVT_GLOBAL_ALLOC:
            // 分配指令
            CompileAlloc(value);
            break;
        case KOOPA_RVT_LOAD:
            // 加载指令
            CompileLoad(value);
            break;
        case KOOPA_RVT_STORE:
            // 存储指令
            CompileStore(value);
            break;
        case KOOPA_RVT_JUMP:
            // 跳转指令
            o << "j " << real_name(kind.data.jump.target) << std::endl;
            break;
        case KOOPA_RVT_BRANCH:
            // 分支指令
            CompileBranch(value);
            break;
        default:
            // 其他类型暂时遇不到
            debug(kind.tag);
            assert(false);
    }
}

void KoopaParser::CompileReturn(const koopa_raw_value_t &value) {
    debug("CompileReturn");
    const koopa_raw_value_kind_t &kind = value->kind;
    auto &ret = kind.data.ret.value;
    if (ret) {
        load_value(ret, "a0");
    }
    o << "addi sp, sp, " << stack_length << std::endl;
    o << "ret" << std::endl;
}

void KoopaParser::CompileAlloc(const koopa_raw_value_t &value) {
    debug("CompileAlloc");
    /*
    const koopa_raw_value_kind_t &kind = value->kind;
    auto &alloc = kind.data.global_alloc;
    auto &init = alloc.init;
    if (init) {
        load_value(init, "t0");
        store_value(value, "t0");
    } else {
        assert(false);
    }
    */
}

void KoopaParser::CompileLoad(const koopa_raw_value_t &value) {
    debug("CompileLoad");
    const koopa_raw_value_kind_t &kind = value->kind;
    auto &load = kind.data.load;
    auto &ptr = load.src;
    load_value(ptr, "t0");
    store_value(value, "t0");
}

void KoopaParser::CompileStore(const koopa_raw_value_t &value) {
    debug("CompileStore");
    const koopa_raw_value_kind_t &kind = value->kind;
    auto &store = kind.data.store;
    auto &ptr = store.dest;
    auto &src = store.value;
    load_value(src, "t0");
    store_value(ptr, "t0");
}

void KoopaParser::CompileBranch(const koopa_raw_value_t &value) {
    debug("CompileBranch");
    const koopa_raw_value_kind_t &kind = value->kind;
    auto &branch = kind.data.branch;
    auto &cond = branch.cond;
    auto &fbb = branch.false_bb;
    auto &tbb = branch.true_bb;
    load_value(cond, "t0");
    o << "beqz t0, " << real_name(fbb) << std::endl;
    o << "j " << real_name(tbb) << std::endl;
}

void KoopaParser::CompileBinary(const koopa_raw_value_t &value) {
    debug("CompileBinary");
    const koopa_raw_value_kind_t &kind = value->kind;
    auto &bin = kind.data.binary;
    auto &lhs = bin.lhs;
    auto &rhs = bin.rhs;
    load_value(lhs, "t0");
    load_value(rhs, "t1");
    switch (bin.op) {
        case KOOPA_RBO_ADD:
            o << "add t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_SUB:
            o << "sub t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_MUL:
            o << "mul t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_DIV:
            o << "div t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_MOD:
            o << "rem t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_EQ:
            o << "sub t0, t0, t1" << std::endl;
            o << "seqz t0, t0" << std::endl;
            break;
        case KOOPA_RBO_NOT_EQ:
            o << "sub t0, t0, t1" << std::endl;
            o << "snez t0, t0" << std::endl;
            break;
        case KOOPA_RBO_GT:
            o << "slt t0, t1, t0" << std::endl;
            break;
        case KOOPA_RBO_LT:
            o << "slt t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_GE:
            o << "slt t0, t0, t1" << std::endl;
            o << "seqz t0, t0" << std::endl;
            break;
        case KOOPA_RBO_LE:
            o << "slt t0, t1, t0" << std::endl;
            o << "seqz t0, t0" << std::endl;
            break;
        case KOOPA_RBO_AND:
            o << "and t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_OR:
            o << "or t0, t0, t1" << std::endl;
            break;
        case KOOPA_RBO_XOR:
            o << "xor t0, t0, t1" << std::endl;
            break;
        default:
            // 不支持移位, SysY生成的代码不会遇到
            assert(false);
    }
    store_value(value, "t0");
}
