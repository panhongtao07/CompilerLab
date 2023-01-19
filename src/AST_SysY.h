#ifndef __SYSY_AST_H__
#define __SYSY_AST_H__

// 不写IDE报错很多，太难看了，还是写上吧
#include <iostream>
#include <unordered_map>
#include <memory>
#include <cassert>

#define record_frame(T) 

#define prepare_expr(expr) \
    do { \
        if (!(expr)->is_value()) { \
            o << *(expr); \
        } \
    } while (0)

// 方法设置表达式的值, 并同步共享指针表
// final 的表达式不再具有父表达式, 解析到此结束, 会被设置为临时值并立刻计算
#define set_method(expr, method_name, final, type) \
    /* 用于设置表达式的值, 并同步共享指针表 */ \
    void method_name(type* expr, bool _final = final) { \
        if (_final) expr->set_value_as_temp(); \
        this->expr = share_table->get_or_create<type>(expr); \
    }

#define define_with_set_method(expr, method_name, final, type) \
    ptr<type> expr; \
    set_method(expr, method_name, final, type)

#define define_expr_with_set_method(expr, method_name, final) \
    define_with_set_method(expr, method_name, final, ExpAST)

#define USE_VAR_NAME 1

class BaseAST;
class ExpAST;
class ValueAST;

class SharedTable;
namespace {
    template <typename T>
    using ptr = std::shared_ptr<T>;
    using Table = std::unordered_map<std::string, ValueAST*>;
};

inline int var_count = 0;
// 共享指针表
inline std::unique_ptr<SharedTable> share_table = std::make_unique<SharedTable>();
// 符号表用于解析时记录符号, 帮助建立 AST 的连接, 编译时不使用
inline std::unique_ptr<Table> symbol_table = std::make_unique<Table>();

// 获取符号表中的变量，若不存在则返回空指针
ValueAST* get_var(const std::string& name);

// 管理类型的类
// 表达式共享指针表，用于将 AST 中的指针替换为共享指针
class SharedTable {
private:
    std::unordered_map<ExpAST*, ptr<ExpAST>> _table;
public:
    // 获取当前共享指针或创建
    ptr<ExpAST> get_or_create(ExpAST* key);
    // 获取当前共享指针或创建, 为什么不能在 cpp 定义呢?
    template <typename T>
    ptr<T> get_or_create(ExpAST* key)  {
        auto result = get_or_create(key);
        return std::reinterpret_pointer_cast<T>(result);
    }
};


// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;
    // 打印 AST 的结构
    virtual std::ostream& dump(std::ostream& o = std::cout) const = 0;
    // 编译 AST 并输出 Koopa IR
    virtual std::ostream& compile(std::ostream& o = std::cout) const = 0;
    friend std::ostream& operator<<(std::ostream& o, const BaseAST& a) {
        return a.compile(o);
    }
};

class CompUnitAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_def;
    record_frame(CompUnitAST)

    std::ostream& dump(std::ostream& o = std::cout) const override {
        return o << "CU {" << *func_def << "}";
    }
    std::ostream& compile(std::ostream& o = std::cout) const override {
        return o << *func_def;
    }
};

class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string identifier;
    std::unique_ptr<BaseAST> block;

    record_frame(FuncDefAST)

    std::ostream& dump(std::ostream& o = std::cout) const override {
        return o << "FD {" << *func_type << ", " << identifier << ", " << *block
                 << "}";
    }
    std::ostream& compile(std::ostream& o = std::cout) const override {
        o << "fun @" << identifier << *func_type << " {" << std::endl;
        return o << *block << "}" << std::endl;
    }
};

class FuncTypeAST : public BaseAST {
public:
    std::string identifier;

    record_frame(FuncTypeAST)

    std::ostream& dump(std::ostream& o = std::cout) const override {
        return o << "FT {" << identifier << "}";
    }
    std::ostream& compile(std::ostream& o = std::cout) const override {
        o << "()";
        if (!identifier.empty())
            o << ": i32";
        return o;
    }
};

class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> statement;

    record_frame(BlockAST)

    std::ostream& dump(std::ostream& o = std::cout) const override {
        return o << "B {" << *statement << "}";
    }
    std::ostream& compile(std::ostream& o = std::cout) const override {
        return o << "%entry:" << std::endl << *statement;
    }
};

// 表达式的基类
class ExpAST: public BaseAST {
public:
    record_frame(ExpAST)
    // 该类为单个值还是表达式
    virtual bool is_value() const {return false;}
    // 确定表达式结束后, 生成表达式及其子表达式的所有临时符号
    virtual void set_value_as_temp() = 0;
    // 获取表达式结果值
    virtual const ValueAST* value_ptr() const = 0;
};

// 单值或符号
class ValueAST : public ExpAST {
public:
    enum class Type {
        // 默认值：未定义
        Undef,
        Num,
        Temp,
        Var,
    } type;
    int number;
    // 变量名, 没有使用意义
#if USE_VAR_NAME
    std::string identifier;
#endif

    ValueAST() : type(Type::Undef) {}
    ValueAST(int number) : type(Type::Num), number(number) {}
#if USE_VAR_NAME
    ValueAST(const std::string& identifier) : type(Type::Var), identifier(identifier) {}
#else
    ValueAST(const std::string& identifier) : type(Type::Var) {}
#endif
    record_frame(ValueAST)

    std::ostream& dump(std::ostream& o = std::cout) const override {
        switch (type) {
            case Type::Num:
                return o << number;
            case Type::Temp:
                return o << "T {" << number << "}";
            case Type::Var:
            #if USE_VAR_NAME
                return o << "V {" << identifier << number << "}";
            #else
                return o << "V {" << number << "}";
            #endif
            default:
                assert(false);
        }
    }
    std::ostream& compile(std::ostream& o = std::cout) const override {
        switch (type) {
            case Type::Num:
                return o << number;
            case Type::Temp:
                return o << "%" << number;
            case Type::Var:
            #if USE_VAR_NAME
                return o << "@" << identifier;
            #else
                return o << "@v" << number;
            #endif
            default:
                assert(false);
        }
    }

    bool is_value() const override {
        return true;
    }

    void set_value_as_temp() override {
        if (type == Type::Undef) {
            type = Type::Temp;
            number = var_count++;
        }
    }

    const ValueAST* value_ptr() const override {
        return this;
    }
};

// 读取具名变量值
// 由于 Koopa IR 不允许多赋值, 实际上是一个表达式而不是单值
class GetVarAST : public ExpAST {
public:
    define_with_set_method(var, set_var, 0, ValueAST)
    define_with_set_method(temp, set_res, 1, ValueAST)

    GetVarAST(ValueAST* var) {
        set_var(var);
    }
    record_frame(GetVarAST)

    std::ostream& dump(std::ostream& o = std::cout) const override {
        return o << "V.GET {" << *var << "->" << *temp << "}";
    }

    std::ostream& compile(std::ostream& o = std::cout) const override {
        return o << *temp << " = load " << *var << std::endl;
    }

    void set_value_as_temp() override {
        assert(!temp);
        set_res(new ValueAST());
    }

    const ValueAST* value_ptr() const override {
        return temp.get();
    }
};

// 二元运算
class BinaryAST : public ExpAST {
public:
    // SysY 语言支持的运算符不包含位运算
    enum class Type {
        // 默认值：未定义
        Undef,
        // 算术运算
        Add,
        Sub,
        Mul,
        Div,
        Mod,
        // 逻辑运算
        And,
        Or,
        Xor,
        // 比较运算
        Eq,
        Ne,
        Lt,
        Gt,
        Lte,
        Gte,
    } type;
    const static std::string type_str[];
    const static std::string inst_str[];
    const static std::string op_str[];

    define_expr_with_set_method(l_val, set_left, 0)
    define_expr_with_set_method(r_val, set_right, 0)
    define_with_set_method(result, set_result, 1, ValueAST)

    // 在编译前, 由语义分析器设置包含结果在内的所有信息
    BinaryAST() : type(Type::Undef) {}
    record_frame(BinaryAST)

    const std::string& get_type_str() const {
        return type_str[(int)type];
    }

    const std::string& get_inst_str() const {
        return inst_str[(int)type];
    }

    std::ostream& dump(std::ostream& o = std::cout) const override {
        assert(type != Type::Undef);
        assert(result);
        if (result->type == ValueAST::Type::Num) {
            return o << *result;
        }
        return o << get_type_str() << " {" << *l_val << ", " << *r_val << "}";
    }

    std::ostream& compile(std::ostream& o = std::cout) const override {
        assert(type != Type::Undef);
        assert(result);
        // 如果已经是常量, 则不需要生成指令
        if (result->type == ValueAST::Type::Num) {
            return o;
        }
        // 先生成子表达式
        prepare_expr(l_val);
        prepare_expr(r_val);
        // 生成运算指令
        o << *result << " = " << get_inst_str() << " " << *l_val->value_ptr()
          << ", " << *r_val->value_ptr() << std::endl;
        return o;
    }

    void set_value_as_temp() override {
        // 如果已经计算出常量, 返回
        if (result != nullptr && result->type == ValueAST::Type::Num) {
            return;
        }
        // 等待子节点计算常量
        l_val->set_value_as_temp();
        r_val->set_value_as_temp();
        // 如果子节点都是常量, 则计算结果
        if (l_val->value_ptr()->type == ValueAST::Type::Num &&
            r_val->value_ptr()->type == ValueAST::Type::Num) {
            int l = l_val->value_ptr()->number;
            int r = r_val->value_ptr()->number;
            int res;
            switch (type) {
                case Type::Add: res = l + r;        break;
                case Type::Sub: res = l - r;        break;
                case Type::Mul: res = l * r;        break;
                case Type::Div: res = l / r;        break;
                case Type::Mod: res = l % r;        break;
                case Type::And: res = !!l && !!r;   break;
                case Type::Or:  res = !!l || !!r;   break;
                case Type::Xor: res = !!l ^ !!r;    break;
                case Type::Eq:  res = l == r;       break;
                case Type::Ne:  res = l != r;       break;
                case Type::Lt:  res = l < r;        break;
                case Type::Gt:  res = l > r;        break;
                case Type::Lte: res = l <= r;       break;
                case Type::Gte: res = l >= r;       break;
                default:
                    assert(false);
            }
            set_result(new ValueAST(res));
        } else {
            // 当前逻辑下分析器不再逐步生成临时变量, 不应提前确定结果
            assert(!result);
            set_result(new ValueAST());
        }
    }

    const ValueAST* value_ptr() const override {
        return result.get();
    }
};

// 语句的基类
class StmtAST : public BaseAST {
public:
    std::unique_ptr<StmtAST> next;

    record_frame(StmtAST)

    // 将语句添加到链表末尾
    void push_back(StmtAST* stmt) {
        if (next) {
            next->push_back(stmt);
        } else {
            next.reset(stmt);
        }
    }

    // 打印本语句的结构
    virtual std::ostream& dump_this(std::ostream& o = std::cout) const = 0;
    // 编译本语句并输出 Koopa IR
    virtual std::ostream& compile_this(std::ostream& o = std::cout) const = 0;
    std::ostream& dump(std::ostream& o = std::cout) const override final {
        dump_this(o);
        if (next) {
            next->dump(o);
        }
        return o;
    }
    std::ostream& compile(std::ostream& o = std::cout) const override final {
        compile_this(o);
        if (next) {
            next->compile(o);
        }
        return o;
    }
};

// 返回语句
class ReturnAST : public StmtAST {
public:
    define_expr_with_set_method(expr, set_expr, 1)
    record_frame(ReturnAST)

    std::ostream& dump_this(std::ostream& o = std::cout) const override {
        if (expr) {
            prepare_expr(expr);
            o << "return " << *expr->value_ptr() << ";";
        } else {
            o << "return;";
        }
        return o << "return " << *expr << ";";
    }
    std::ostream& compile_this(std::ostream& o = std::cout) const override {
        if (expr) {
            prepare_expr(expr);
            o << "ret " << *expr->value_ptr() << std::endl;
        } else {
            o << "ret" << std::endl;
        }
        return o;
    }
};

// 声明语句
class DeclAST : public StmtAST {
public:
    define_with_set_method(var, set_var, 0, ValueAST)

    record_frame(DeclAST)

    // 设置属性值并同步到符号表和共享指针表, 符号表此前必须不存在该变量
    void set_var_and_sync(const std::string& name, ValueAST* value) {
        assert(!get_var(name));
        set_var(value);
        (*symbol_table)[name] = value;
    }
};

// 常量声明语句
class ConstDeclAST : public DeclAST {
public:
    std::string name;

    ConstDeclAST(const std::string& name, ValueAST* value) : name(name) {
        assert(value->type == ValueAST::Type::Num);
#if USE_VAR_NAME
        // 记录名称时, 生成新的 ValueAST, 并在其中记录名称
        value = new ValueAST(value->number);
        value->identifier = name;
#endif
        set_var_and_sync(name, value);
    }
    record_frame(ConstDeclAST)
    std::ostream& dump_this(std::ostream& o = std::cout) const override {
        return o << "const decl " << name << " = " << *var << ";";
    }
    // 常量被自动替换为值, 无需产生中间代码
    std::ostream& compile_this(std::ostream& o = std::cout) const override {
        return o;
    }
};

// 变量声明语句
class VarDeclAST : public DeclAST {
public:
    VarDeclAST(const std::string& name) {
        set_var_and_sync(name, new ValueAST(name));
    }
    record_frame(VarDeclAST)
    std::ostream& dump_this(std::ostream& o = std::cout) const override {
        return o << "decl " << *var << ";";
    }
    std::ostream& compile_this(std::ostream& o = std::cout) const override {
        return o << *var << " = alloc i32" << std::endl;
    }
};
#endif
