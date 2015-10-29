#ifndef H_LEX
#define H_LEX

#include <sstream>

#include <llvm/IR/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Analysis/Passes.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"

namespace rift {


struct Token {
    enum class Type {
        add,
        sub,
        mul,
        div,
        eq,
        neq,
        lt,
        gt,
        assign,
        opar,
        cpar,
        osbr,
        csbr,
        ocbr,
        ccbr,
        semicolon,
        comma,
        number,
        character,
        ident,
        kwFunction,
        kwIf,
        kwElse,
        kwWhile,
        kwC,
        kwLength,
        kwEval,
        kwType,
        eof

    };

    static std::string typeToStr(Type t) {
        switch(t) {
        case Type::add:
            return "+";
        case Type::sub:
            return "-";
        case Type::mul:
            return "*";
        case Type::div:
            return "/";
        case Type::eq:
            return "==";
        case Type::neq:
            return "!=";
        case Type::lt:
            return "<";
        case Type::gt:
            return ">";
        case Type::assign:
            return "<-";
        case Type::opar:
            return "(";
        case Type::cpar:
            return ")";
        case Type::osbr:
            return "[";
        case Type::csbr:
            return "]";
        case Type::ocbr:
            return "{";
        case Type::ccbr:
            return "}";
        case Type::semicolon:
            return ";";
        case Type::comma:
            return ",";
        case Type::number:
            return "number";
        case Type::character:
            return "character";
        case Type::ident:
            return "identifier";
        case Type::kwFunction:
            return "keyword function";
        case Type::kwIf:
            return "keyword if";
        case Type::kwElse:
            return "keyword else";
        case Type::kwWhile:
            return "keyword while";
        case Type::kwC:
            return "keyword c";
        case Type::kwLength:
            return "keyword length";
        case Type::kwEval:
            return "keyword eval";
        case Type::kwType:
            return "keyword type";
        case Type::eof:
            return "EOF";
        default:
            return "unknown";
        }
    }

    Type type;
    union {
        // actual double value
        double d;
        // index to constant pool where the string is stored
        unsigned c;
    };
    Token(Type t):
        type(t) {}

    Token(double value):
        type(Type::number),
        d(value) {}

    Token(Type t, int index):
        type(t),
        c(index) {}

    bool operator == (Token::Type t) {
        return type == t;
    }

    bool operator != (Token::Type t) {
        return type != t;
    }
};








class Lexer {
public:


    void scan(std::string const & input) {
        std::stringstream ss(input);
        scan(ss);
    }

    void scan(std::istream & input) {
        while (not input.eof())
            tokens_.push_back(readToken(input));
        if (tokens_.back() != Token::Type::eof)
            tokens_.push_back(Token::Type::eof);
    }

    static std::string const & getPoolObject(unsigned index) {
        return pool_[index];
    }

    bool eof() {
        return tidx_ >= tokens_.size() - 1;
    }
    Token top() {
        return tokens_[tidx_];
    }

    Token pop() {
        if (eof())
            return tokens_[tidx_];
        else
            return tokens_[tidx_++];
    }


private:
    Token readToken(std::istream & input) {
        char c = input.get();
        // skip the whitespace
        while (c == ' ' or c == '\n' or c == '\t')
            c = input.get();
        if (input.eof())
            return Token(Token::Type::eof);
        switch (c) {
        case '+':
             return Token(Token::Type::add);
        case '-':
            return Token(Token::Type::sub);
        case '*':
            return Token(Token::Type::mul);
        case '/':
            return Token(Token::Type::div);
        case '(':
            return Token(Token::Type::opar);
        case ')':
            return Token(Token::Type::cpar);
        case '[':
            return Token(Token::Type::osbr);
        case ']':
            return Token(Token::Type::csbr);
        case '{':
            return Token(Token::Type::ocbr);
        case '}':
            return Token(Token::Type::ccbr);
        case '<':
            if (input.peek() == '-') {
                input.get();
                return Token(Token::Type::assign);
            }
            return Token(Token::Type::lt);
        case '>':
            return Token(Token::Type::gt);
        case ';':
            return Token(Token::Type::semicolon);
        case ',':
            return Token(Token::Type::comma);
        case '=':
            if (input.peek() != '=')
                return Token(Token::Type::assign);
            return Token(Token::Type::eq);
        case '!':
            if (input.get() != '=')
                throw "Expected != but only ! found";
            return Token(Token::Type::neq);
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return number(c - '0', input);
        case '.':
            return fractionNumber(0, input);
        case '"':
            return stringLiteral(input);
        default:
            return identOrKeyword(c, input);
        }
    }

    bool isNumber(char c) {
        return c >= '0' and c <= '9';
    }

    bool isLetter(char c) {
        return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_';
    }

    Token number(int n, std::istream & input) {
        while (true) {
            char c = input.peek();
            if (isNumber(c)) {
                input.get();
                n = n * 10 + (c - '0');
            } else if (c == '.') {
                input.get();
                return fractionNumber(n, input);
            } else {
                return Token(n);
            }
        }
    }

    Token fractionNumber(double n, std::istream & input) {
        double d = 10;
        while (true) {
            char c = input.peek();
            if (isNumber(c)) {
                input.get();
                n = n + ((c - '0') / d);
                d = d * 10;
            } else {
                if (d == 10)
                    throw "At least one digit must be present after dot";
                return Token(n);
            }
        }
    }


    Token identOrKeyword(char start, std::istream & input) {
        std::string x(1, start);
        while (isNumber(input.peek()) or isLetter(input.peek()))
            x += input.get();
        if (x == "function")
            return Token(Token::Type::kwFunction);
        else if (x == "if")
            return Token(Token::Type::kwIf);
        else if (x == "else")
            return Token(Token::Type::kwElse);
        else if (x == "while")
            return Token(Token::Type::kwWhile);
        else if (x == "c")
            return Token(Token::Type::kwC);
        else if (x == "eval")
            return Token(Token::Type::kwEval);
        else if (x == "length")
            return Token(Token::Type::kwLength);
        else if (x == "type")
            return Token(Token::Type::kwType);
        else
            return Token(Token::Type::ident, addToPool(x));
    }

    Token stringLiteral(std::istream & input) {
        std::string x;
        while (input.peek() != '"')
            x += input.get();
        input.get(); // closing "
        return Token(Token::Type::character, addToPool(x));
    }


    static int addToPool(std::string const & s) {
        for (unsigned i = 0; i < pool_.size(); ++i)
            if (pool_[i] == s)
                return i;
        pool_.push_back(s);
        return pool_.size() - 1;
    }


    std::vector<Token> tokens_;

    unsigned tidx_ = 0;

    static std::vector<std::string> pool_;




};

} // namespace rift

#endif
