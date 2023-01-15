#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include "koopa.h"
#include "koopa_parser.h"
#include "AST_SysY.h"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// sysy.tab.hpp 不包含 yyin 的定义，且并不总是存在，所以不使用include方式
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
    // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
    // compiler 模式 输入文件 -o 输出文件
    assert(argc == 5);
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];

    assert(mode[1] == 'k' || mode[1] == 'r' || mode[1] == 'p' || mode[1] == 'v');
    auto k2r = (mode[1] != 'k');

    // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
    yyin = fopen(input, "r");
    assert(yyin);

    // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    // 使用 ss 可能导致更大的内存代价, 但能减少时间代价并防止输出不一致
    stringstream ss;
    ss << *ast << endl;
    auto s = ss.str();

    // 第一阶段: 将 SysY 代码转换为 Koopa IR
    // 如果不转化koopa, 则输出解析得到的 AST 后结束
    if (!k2r) {
        if (output) {
            ofstream ofs(output);
            ofs << s;
        }
        cout << "Koopa:\n------\n";
    }

    // 输出解析得到的 AST
    cout << s;
    if (!k2r) {
        return 0;
    }

    // 第二阶段: 将 Koopa IR 转换为 RISC-V 汇编代码
    // 第一步：将 Koopa IR 转换为内存形式 raw program
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t err_code = koopa_parse_from_string(s.c_str(), &program);
    assert(err_code == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    ss = stringstream();
    auto parser = KoopaParser(ss);
    // 访问 raw program, 生成 raw program 的文本表示
    if (mode[1] == 'v') {
        cout << "Raw:\n----\n";
        KoopaParser(cout).Visit(raw);
    }

    // 第二步：将 raw program 转换为 RISC-V 汇编代码
    // 生成 RISCV 汇编代码
    parser.Compile(raw);

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);

    // 输出
    s = ss.str();
    if (output) {
        ofstream ofs(output);
        ofs << s;
    }
    cout << "RISCV:\n----\n";
    cout << s;

    return 0;
}
