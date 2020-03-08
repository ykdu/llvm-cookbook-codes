/*************************************************************************
	> File Name: toy.cpp
	> Author: Du Yunkai
	> Mail: duyunkai@huawei.com
	> Created Time: Sun Mar  8 22:43:21 2020
 ************************************************************************/

/* automata中的状态 */
enum Token_Type {
    EOF_TOKEN = 0,      // 文件结束
    NUMERIC_TOKEN,      // 当前token是numeric
    IDENTIFIER_TOKEN,   // 当前token是identifier
    PARAN_TOKEN,        // .........是圆括号
    DEF_TOKEN           // .........是def关键字
};

static int Numeric_Val; // numeric值
static std::string Identifier_string; // identifier值

/* 截取出一个token，并返回它对应的automata状态。(side-effect: file指针后移) */
static int get_token() {
    static int LastChar = ' ';  // 防止输入为空

    while(isspace(LastChar))
        LastChar = fgetc(file); // 过滤空格，找token首字母

    if(isalpha(LastChar)) { // 首字母是字符 --may be--> def / identifier
        Identifier_string = LastChar;
        while(isalnum((LastChar = fgetc(file)))) {
            Identifier_string += LastChar;
        }
        if(Identifier_string == "def")
            return DEF_TOKEN;
        return IDENTIFIER_TOKEN;
    }

    if(isdigit(LastChar)) { // 首字母是数字 --must be--> numeric
        std::string NumStr = LastChar;
        while(isdigit((LastChar = fgetc(file)))) {
            NumStr += LastChar;
        }
        Numeric_Val = strtod(NumStr.c_str(), 0);
        return NUMERIC_TOKEN;
    }

    if(LastChar == '#') { // 首字母是# --must be-->注释，返回下一个token
        do {
            LastChar = fgetc(file);
        } while(LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if(LastChar != EOF)
            return get_token();
    }

    if(LastChar == EOF)
        return EOF_TOKEN;

    int ThisChar = LastChar;
    LastChar = fgetc(file);
    return ThisChar;        // TODO:1.不太理解，2.为什么不给括号一个Token
}

/*********************** 定义AST **********************/

class BaseAST {
    public:
        virtual ~BaseAST();
}

/* Variable节点 */
class VariableAST: public BaseAST {
    std::string Var_Name;
    public:
    VariableAST(std::string &name): Var_Name(name) {
    }
}

/* Numeric节点 */
class NumericAST: public BaseAST {
    int numeric_val;
    public:
    NumericAST(int val): numeric_val(val) {
    }
}

/* 二元expr节点 */
class BinaryAST: public BaseAST {
    std::string Bin_Operator;
    BaseAST *LHS, *RHS;
    public:
    BinaryAST(std::string op, BaseAST *lhs, BaseAST *rhs): Bin_Operator(op), LHS(lhs), RHS(rhs) {
    }
}

/* 函数声明（函数名+参数列表）节点 */
class FunctionDeclAST {
    std::string Func_Name;
    std::vector<std::string> Arguments;
    public:
    FunctionDeclAST(const std::string &name, const std::vector<std::string> &args): Func_Name(name), Arguments(args) {
    }
}

/* 函数定义（函数声明+函数体）节点*/
class FunctionDefAST {
    FunctionDeclAST *Func_Decl;
    BaseAST* Body;
    public:
    FunctionDefAST(FunctionDeclAST *proto, BaseAST *body): Func_Decl(proto), Body(body) {
    }
}

/* 函数调用（函数名+参数列表，且参数可以是任意节点）节点 */
class FunctionCallAST: public BaseAST {
    std::string Function_Callee;
    std::vector<BaseAST*> Function_Arguments;
    public:
    FunctionCallAST(const std::string &callee, std::vector<BaseAST*> &args): Function_Callee(callee), Function_Arguments(args) {
    }
}

static in Current_token;
static void next_token() {
    Current_token = get_token();
}

/***************************** Parser构造AST ******************************/
/* 不断接受token，生成AST节点*/
static BaseAST* Base_parser() {
    switch(Current_token) {
        case NUMERIC_TOKEN: return numeric_parser();
        case IDENTIFIER_TOKEN: return identifier_parser();
        case '(': return paran_parser();
        default: return 0;
    }
}

// TODO: 返回值类型是NumericAST？
// numeric_expr := number
static BaseAST* numeric_parser() {  // NUMERIC_TOKEN --must be--> Numeric AST节点
    BaseAST *Result = new NumericAST(Numeric_Val);
    next_token();
    return Result;
}

// identifier_expr
//     := identifier
//     := identifier '('expr_list ')'
static BaseAST* identifier_parser() {   // IDENTIFIER_TOKEN --may be--> Variable / FunctionCall AST节点 (后文相关，取决于后一个是否是括号)
    std::string IdName = Identifier_string;
    next_token();

    if(Current_token != '(')
        return new VariableAST(IdName);

    next_token(); // 跳过'('
    std::vector<BaseAST*> Args;
    if(Current_token != ')') {
        while(1) {
            BaseAST* Arg = expression_parser();
            if(!Arg)
                return 0; // TODO: 异常情况在哪儿捕获？
            Args.push_back(Arg);

            if(Current_token != ',')
                return 0;
            next_token();
        }
    }
    next_token(); // 跳过')'
    return new FunctionCallAST(IdName, Args);
}

// 
static FunctionDeclAST *func_decl_parser() {
    if(Current_token != IDENTIFIER_TOKEN)
        return 0;

    std::string FnName = Identifier_string;
    next_token;

    if(Current_token != '(')
        return 0;

    std::vector<std::string> Function_Argument_Names;
    while(next_token() == IDENTIFIER_TOKEN)
        Function_Argument_Names.push_back(Identifier_string);

    if(Current_token != ')')
        return 0;

    next_token;
    return FunctionDeclAST(FnName, Function_Argument_Names);
}

static FunctionDefAST *func_def_parser() {
    next_token(); // 跳过'def'
    FunctionDeclAST *Decl = func_decl_parser();
    if(Decl == 0)
        return 0;

    if(BaseAST* Body = expression_parser())
        return new FunctionDefAST(Decl, Body);

    return 0;
}

static BaseAST* expression_parser() {
    BaseAST *LHS = Base_Parser();
    if(!LHS)
        return 0;
    return binary_op_parser(0, LHS);
}
