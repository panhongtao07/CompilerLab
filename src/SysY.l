%option noyywrap
%option nounput
%option noinput

%{
#include <cstdlib>
#include <string>

// 由 Bison 生成的头文件
#include "SysY.tab.hpp"

using namespace std;
%}

/* 块注释对应的状态，减少匹配复杂度 */
%start BLOCK_COMMENT

/* 空白符和注释 */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*
BlockComment  "/*"[^*]*"*"+([^*/][^*]*"*"+)*"/"

/* 标识符 */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

%%

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* 忽略, 不做任何操作 */ }
{BlockComment}  { /* 忽略, 不做任何操作 */ }

"int"           { return INT; }
"const"         { return CONST; }
"return"        { return RETURN; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENTIFIER; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

.               { /* 未匹配的单个字符直接返回 */ return yytext[0]; }

%%
