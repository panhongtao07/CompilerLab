#ifndef __SYSY_AST_H__
#define __SYSY_AST_H__

// 不写IDE报错很多，太难看了，还是写上吧
#include <iostream>
#include <memory>

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual std::ostream& dump(std::ostream& o = std::cout) const = 0;
    friend std::ostream& operator<<(std::ostream& o, const BaseAST& a) {
        return a.dump(o);
    }
};

class CompUnitAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_def;

    std::ostream& dump(std::ostream& o = std::cout) const override {
        return o << "CU {" << *func_def << "}";
    }
};

class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string identifier;
    std::unique_ptr<BaseAST> block;

    std::ostream& dump(std::ostream& o = std::cout) const override {
        return o << "FD {" << *func_type << ", " << identifier << ", " << *block
                 << "}";
    }
};

class FuncTypeAST : public BaseAST {
public:
    std::string identifier;

    std::ostream& dump(std::ostream& o = std::cout) const override {
        return o << "FT {" << identifier << "}";
    }
};

class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> statement;

    std::ostream& dump(std::ostream& o = std::cout) const override {
        return o << "B {" << *statement << "}";
    }
};

class StmtAST : public BaseAST {
public:
    int number;
    std::ostream& dump(std::ostream& o = std::cout) const override {
        return o << "return " << number << ";";
    }
};
#endif
