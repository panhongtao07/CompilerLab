#include "AST_SysY.h"

// 全局变量

// 初始化类静态成员
const std::string BinaryAST::type_str[] = {
    "UNDEF",
    // 算术运算
    "ADD", "SUB", "MUL", "DIV", "MOD",
    // 逻辑运算
    "AND", "OR", "XOR",
    // 比较运算
    "EQ", "NE", "LT", "GT", "LTE", "GTE",
};
const std::string BinaryAST::inst_str[] = {
    "_undef_",
    // 算术运算
    "add", "sub", "mul", "div", "mod",
    // 逻辑运算
    "and", "or", "xor",
    // 比较运算
    "eq", "ne", "lt", "gt", "le", "ge",
};
const std::string BinaryAST::op_str[] = {
    "_undef_",
    // 算术运算
    "+", "-", "*", "/", "%",
    // 逻辑运算
    "&", "|", "^",
    // 比较运算
    "==", "!=", "<", ">", "<=", ">=",
};

