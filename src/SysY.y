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
    StmtAST *stmt_val;
    ExpAST *exp_val;
    DomainGuard* guard_val;
}

// lexer 返回的所有 token 种类的声明
// <xxx> 表示该符号的返回值，对应于YYSTYPE的哪个属性，降低编写代价
// 增加这个符号后，所有对该符号的访问自动修改为.xxx
%token INT CONST IF ELSE RETURN
%token <str_val> IDENTIFIER
%token <int_val> INT_CONST

// 非终结符的类型定义
// 具有返回类型的似乎必须声明type，意义同token部分
%type <ast_val> FuncDef FuncType Block
%type <stmt_val> Decl ConstDecl VarDecl ConstDef VarDef BlockItem Stmt MatchedStmt OpenStmt
%type <exp_val> Exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp Number
%type <exp_val> ConstInitVal InitVal ConstExp LVal
%type <int_val> UnaryOp MulOp AddOp RelOp EqOp
%type <guard_val> DomainBegin

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

Decl
    : ConstDecl
    | VarDecl
    ;

ConstDecl
    : CONST BType ConstDef ';' { $$ = $3; }
    ;

// 表达式暂不考虑 int 之外的类型, 无需记录类型
BType
    : INT
    ;

ConstDef
    : IDENTIFIER '=' ConstInitVal {
        $$ = new ConstDeclAST(*$1, const_cast<ValueAST*>($3->value_ptr()));
    }
    | ConstDef ',' ConstDef {
        $1->next = unique_ptr<StmtAST>($3);
        $$ = $1;
    }
    ;

ConstInitVal
    : ConstExp      { $$ = const_cast<ValueAST*>($1->value_ptr()); }
    ;

VarDecl
    : BType VarDef ';' { $$ = $2; }
    ;

VarDef
    : IDENTIFIER    { $$ = new VarDeclAST(*$1); }
    | IDENTIFIER '=' InitVal {
        auto decl = new VarDeclAST(*$1);
        auto assign = new AssignAST(*$1);
        assign->set_expr($3);
        decl->next = unique_ptr<StmtAST>(assign);
        $$ = decl;
    }
    | VarDef ',' VarDef {
        // 变量定义可能解释为多个语句, 如声明和赋值
        // 使用push_back保证顺序正确
        $1->push_back($3);
        $$ = $1;
    }
    ;

InitVal
    : Exp           { $$ = $1; }
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
    : '{' DomainBegin BlockItem '}' {
        // 块的作用域结束后, 自动销毁作用域
        auto guard = unique_ptr<DomainGuard>($2);
        auto ast = new BlockAST();
        ast->statement = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    | '{' '}'       {
        $$ = new BlockAST();
    }
    ;

DomainBegin : {
        $$ = new DomainGuard();
    };

BlockItem
    : Stmt          { $$ = $1; }
    | Decl          { $$ = $1; }
    | BlockItem BlockItem {
        $1->push_back($2);
        $$ = $1;
    }
    ;

Stmt
    : MatchedStmt   { $$ = $1; }
    | OpenStmt      { $$ = $1; }
    ;

MatchedStmt
    : RETURN Exp ';' {
        auto ast = new ReturnAST();
        ast->set_expr($2);
        $$ = ast;
    }
    | RETURN ';'     { $$ = new ReturnAST(); }
    | IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
        $$ = new BranchAST($3, $5, $7);
    }
    | Block     { $$ = new BlockStmtAST($1); }
    | Exp ';' {
        auto ast = new ExpStmtAST();
        ast->set_expr($1);
        $$ = ast;
    }
    | ';'       { $$ = new ExpStmtAST(); }
    | LVal '=' Exp ';' {
        auto ast = new AssignAST();
        auto lval = reinterpret_cast<GetVarAST*>($1);
        ast->set_var(const_cast<ValueAST*>(lval->var.get()));
        ast->set_expr($3);
        delete lval;
        $$ = ast;
    }
    ;

OpenStmt
    : IF '(' Exp ')' MatchedStmt ELSE OpenStmt {
        $$ = new BranchAST($3, $5, $7);
    }
    | IF '(' Exp ')' Stmt {
        $$ = new BranchAST($3, $5);
    }
    ;

Exp
    : LOrExp        { $$ = $1; }
    ;

ConstExp
    : Exp {
        $1->set_value_as_temp();
        assert($1->value_ptr()->type == ValueAST::Type::Num);
        $$ = const_cast<ValueAST*>($1->value_ptr());
    }
    ;

PrimaryExp
    : '(' Exp ')'   { $$ = $2; }
    | LVal          { $$ = $1; }
    | Number        { $$ = $1; }
    ;

UnaryExp
    : PrimaryExp    { $$ = $1; }
    | UnaryOp UnaryExp { return_binary($1, new ValueAST(0), $2, $$); }
    ;

UnaryOp
    : '+' { $$ = (int)BinaryAST::Type::Add; }
    | '-' { $$ = (int)BinaryAST::Type::Sub; }
    | '!' { $$ = (int)BinaryAST::Type::Eq; }
    ;

MulExp
    : UnaryExp      { $$ = $1; }
    | MulExp MulOp UnaryExp { return_binary($2, $1, $3, $$); }
    ;

MulOp
    : '*' { $$ = (int)BinaryAST::Type::Mul; }
    | '/' { $$ = (int)BinaryAST::Type::Div; }
    | '%' { $$ = (int)BinaryAST::Type::Mod; }
    ;

AddExp
    : MulExp        { $$ = $1; }
    | AddExp AddOp MulExp { return_binary($2, $1, $3, $$); }
    ;

AddOp
    : '+' { $$ = (int)BinaryAST::Type::Add; }
    | '-' { $$ = (int)BinaryAST::Type::Sub; }
    ;

RelExp
    : AddExp        { $$ = $1; }
    | RelExp RelOp AddExp { return_binary($2, $1, $3, $$); }
    ;

RelOp
    : '<'       { $$ = (int)BinaryAST::Type::Lt; }
    | '>'       { $$ = (int)BinaryAST::Type::Gt; }
    | '<' '='   { $$ = (int)BinaryAST::Type::Lte; }
    | '>' '='   { $$ = (int)BinaryAST::Type::Gte; }
    ;

EqExp
    : RelExp        { $$ = $1; }
    | EqExp EqOp RelExp { return_binary($2, $1, $3, $$); }
    ;

EqOp
    : '!' '='   { $$ = (int)BinaryAST::Type::Ne; }
    | '=' '='   { $$ = (int)BinaryAST::Type::Eq; }
    ;

LAndExp
    : EqExp         { $$ = $1; }
    | LAndExp '&' '&' EqExp {
        ExpAST *l, *r;
        return_binary(BinaryAST::Type::Ne, new ValueAST(0), $1, l);
        return_binary(BinaryAST::Type::Ne, new ValueAST(0), $4, r);
        return_binary(BinaryAST::Type::And, l, r, $$);
    }
    ;

LOrExp
    : LAndExp       { $$ = $1; }
    | LOrExp '|' '|' LAndExp {
        ExpAST *l, *r;
        return_binary(BinaryAST::Type::Ne, new ValueAST(0), $1, l);
        return_binary(BinaryAST::Type::Ne, new ValueAST(0), $4, r);
        return_binary(BinaryAST::Type::Or, l, r, $$);
    }
    ;

LVal
    : IDENTIFIER    {
        auto var = symbol_table->get_var(*$1);
        assert(var != nullptr);
        if (var->type == ValueAST::Type::Var) {
            $$ = new GetVarAST(var);
        } else {
            $$ = var;
        }
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
