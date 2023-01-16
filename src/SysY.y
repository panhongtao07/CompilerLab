// ---------------
// |    定义段    |
// ---------------

%code requires {
// YYSTYPE和yyparse等用到的必要头文件
#include <string>
#include "AST_SysY.h"
}

%{
#include <iostream>
#include <memory>
#include <string>
#include "AST_SysY.h"

#define return_binary(op, l, r, res) \
    do { \
        auto exp = new BinaryAST(); \
        exp->type = (BinaryAST::Type)op; \
        exp->set_left(l); \
        exp->set_right(r); \
        res = exp; \
    } while (0)

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 在 union 里不应包含带析构函数的类，除非自行编写析构函数并处理
%union {
    std::string *str_val;
    int int_val;
    BaseAST *ast_val;
    ExpAST *exp_val;
}

// lexer 返回的所有 token 种类的声明
// <xxx> 表示该符号的返回值，对应于YYSTYPE的哪个属性，降低编写代价
// 增加这个符号后，所有对该符号的访问自动修改为.xxx
%token INT RETURN
%token <str_val> IDENTIFIER
%token <int_val> INT_CONST

// 非终结符的类型定义
// 具有返回类型的似乎必须声明type，意义同token部分
%type <ast_val> FuncDef FuncType Block Stmt
%type <exp_val> Number

%%

// ---------------
// |    规则段    |
// ---------------

// 开始符, CompUnit ::= FuncDef
// 大括号后声明了解析完成后 parser 要做的事情
// 使用 $n 访问第n个符号的值，这里 $1
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完开始符, 就说明所有的 token 都被解析了, 即解析结束了
// $1 指代 FuncDef 的返回值, 也就是 $1.str_val 的字符串指针
CompUnit
    : FuncDef {
        auto comp_unit = make_unique<CompUnitAST>();
        comp_unit->func_def = unique_ptr<BaseAST>($1);
        ast = move(comp_unit);
    }
    ;

// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// $$ 表示非终结符的值, 我们可以通过给这个符号赋值的方法来返回结果, 但不会生成return语句
// unique_ptr 可以用来避免内存泄漏, 减少内存管理的负担
FuncDef
    : FuncType IDENTIFIER '(' ')' Block {
        auto ast = new FuncDefAST();
        ast->func_type = unique_ptr<BaseAST>($1);
        ast->identifier = *unique_ptr<string>($2);
        ast->block = unique_ptr<BaseAST>($5);
        $$ = ast;
    }
    ;

FuncType
    : INT {
        auto ast = new FuncTypeAST();
        ast->identifier = "int";
        $$ = ast;
    }
    ;

Block
    : '{' Stmt '}' {
        auto ast = new BlockAST();
        ast->statement = unique_ptr<BaseAST>($2);
        $$ = ast;
    }
    ;

Stmt
    : RETURN Exp ';' {
        auto ast = new ReturnAST();
        ast->set_expr($2);
        $$ = ast;
    }
    ;

Number
    : INT_CONST {
        $$ = new ValueAST($1);
    }
    ;

%%

// ---------------
// |    代码段    |
// ---------------

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
    cerr << "error: " << s << endl;
}
