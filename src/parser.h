#pragma once
#ifndef H_PARSE
#define H_PARSE

#include <ciso646>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "lexer.h"
#include "ast.h"


namespace rift {

    class Parser {
    public:
        ast::Exp * parse(std::string const & str) {
            l_.scan(str);
            std::unique_ptr<ast::Seq> seq(new ast::Seq());
            parseSequenceBody(seq.get());
            return seq.release();
        }

        ast::Exp * parse(std::istream & s) {
            l_.scan(s);
            std::unique_ptr<ast::Seq> seq(new ast::Seq());
            parseSequenceBody(seq.get());
            return seq.release();
        }

    protected:
        Token top() {
            return l_.top();
        }

        Token pop() {
            return l_.pop();
        }

        Token pop(Token::Type t) {
            if (l_.top() == t)
                return l_.pop();
            throw "Syntax error";
        }

        bool condPop(Token::Type t) {
            if (top() == t) {
                pop();
                return true;
            } else {
                return false;
            }
        }

    private:
        /** SEQ ::= '{' { STATEMENT } '}'
            STATEMENT ::= IF | WHILE | EXPRESSION
            EXPRESSION ::= E1 { ( == | != | < | > ) E1 }
            E1 ::= E2 { ( + | - ) E2 }
            E2 ::= E3 { ( * | / ) E3 }
            E3 ::= F { INDEX | CALL | ASSIGNMENT } 
            F ::= NUMBER | STRING | IDENT | FUNCTION | SPECIAL_CALL | '(' EXPRESSION ')'
            CALL ::= '(' [ EXPRESSION {, EXPRESSION } ')'
            INDEX ::= '[' EXPRESSION ']' [ ASSIGNMENT ]
            ASSIGNMENT ::= ( <- | = ) EXPRESSION
            SPECIAL_CALL ::= EVAL | LENGTH | TYPE | C
            EVAL ::= eval '(' EXPRESSION ')'
            LENGTH ::= length '(' EXPRESSION ')'
            TYPE ::= type '(' EXPRESSION ')'
            C ::= c '(' EXPRESSION {, EXPRESSION } ')'
            FUNCTION ::= function '(' [ ident {, ident } ] ')' SEQ
            WHILE ::= while '(' EXPRESSION ')' SEQ
            IF ::= if '(' EXPRESSION ')' SEQ [ else SEQ ]
         */


        ast::Call * parseCall(ast::Exp * variable) {
            std::unique_ptr<ast::UserCall> call(new ast::UserCall(variable));
            pop(Token::Type::opar);
            while (top() != Token::Type::cpar) {
                call->args.push_back(parseExpression());
                if (not condPop(Token::Type::comma))
                    break;
            }
            pop(Token::Type::cpar);
            return call.release();
        }

        ast::SimpleAssignment * parseAssignment(std::unique_ptr<ast::Exp> variable) {
            pop(Token::Type::assign);
            ast::Var * v = dynamic_cast<ast::Var*>(variable.get());
            if (v == nullptr)
                throw "Assignment is only possible into variables";
            std::unique_ptr<ast::SimpleAssignment> assign(new ast::SimpleAssignment(v));
            variable.release();
            assign->rhs = parseExpression();
            return assign.release();
        }

        ast::Exp * parseIndex(ast::Exp * variable) {
            std::unique_ptr<ast::Index> index(new ast::Index(variable));
            pop(Token::Type::osbr);
            index->index = parseExpression();
            pop(Token::Type::csbr);
            if (condPop(Token::Type::assign)) {
                std::unique_ptr<ast::IndexAssignment> ia(new ast::IndexAssignment(index.release()));
                ia->rhs = parseExpression();
                return ia.release();
            } else {
                return index.release();
            }
        }

        ast::Exp * parseEval() {
            pop(Token::Type::kwEval);
            pop(Token::Type::opar);
            std::unique_ptr<ast::Exp> arg(parseExpression());
            pop(Token::Type::cpar);
            return new ast::EvalCall(arg.release());
        }

        ast::Exp * parseLength() {
            pop(Token::Type::kwLength);
            pop(Token::Type::opar);
            std::unique_ptr<ast::Exp> arg(parseExpression());
            pop(Token::Type::cpar);
            return new ast::LengthCall(arg.release());
        }

        ast::Exp * parseType() {
            pop(Token::Type::kwType);
            pop(Token::Type::opar);
            std::unique_ptr<ast::Exp> arg(parseExpression());
            pop(Token::Type::cpar);
            return new ast::TypeCall(arg.release());
        }

        ast::Exp * parseC() {
            pop(Token::Type::kwC);
            pop(Token::Type::opar);
            std::unique_ptr<ast::CCall> result(new ast::CCall());
            do {
                result->args.push_back(parseExpression());
            } while (condPop(Token::Type::comma));
            pop(Token::Type::cpar);
            return result.release();
        }

        ast::Exp * parseF() {
            switch (top().type) {
                case Token::Type::ident:
                    return new ast::Var(pop().c);
                case Token::Type::number:
                    return new ast::Num(pop().d);
                case Token::Type::character:
                    return new ast::Str(pop().c);
                case Token::Type::opar: {
                    pop();
                    std::unique_ptr<ast::Exp> result(parseExpression());
                    pop(Token::Type::cpar);
                    return result.release();
                }
                case Token::Type::kwFunction:
                    return parseFunction();
                case Token::Type::kwEval:
                    return parseEval();
                case Token::Type::kwLength:
                    return parseLength();
                case Token::Type::kwType:
                    return parseType();
                case Token::Type::kwC:
                    return parseC();
                default:
                    throw "literal, variable, call or special call expected";
            }
        }

        ast::Exp * parseE3() {
            std::unique_ptr<ast::Exp> f(parseF());
            while (true) {
                switch (top().type) {
                    case Token::Type::osbr:
                        f.reset(parseIndex(f.release()));
                        break;
                    case Token::Type::opar:
                        f.reset(parseCall(f.release()));
                        break;
                    case Token::Type::assign:
                        f.reset(parseAssignment(std::move(f)));
                        break;
                    default:
                        return f.release();
                }
            }
        }

        ast::Exp * parseE2() {
            std::unique_ptr<ast::Exp> x(parseE3());
            while (true) {
                switch (top().type) {
                case Token::Type::mul:
                case Token::Type::div: {
                    ast::BinExp::Type t;
                    switch (pop().type) {
                    case Token::Type::mul:
                        t = ast::BinExp::Type::mul;
                        break;
                    case Token::Type::div:
                        t = ast::BinExp::Type::div;
                        break;
                    default:
                        assert(false and "unreachable");
                    }
                    x.reset(new ast::BinExp(x.release(), parseE3(), t));
                    break;
                }
                default:
                    return x.release();
                }
            }
        }

        ast::Exp * parseE1() {
            std::unique_ptr<ast::Exp> x(parseE2());
            while (true) {
                switch (top().type) {
                case Token::Type::add:
                case Token::Type::sub: {
                    ast::BinExp::Type t;
                    switch (pop().type) {
                    case Token::Type::add:
                        t = ast::BinExp::Type::add;
                        break;
                    case Token::Type::sub:
                        t = ast::BinExp::Type::sub;
                        break;
                    default:
                        assert(false and "unreachable");
                    }
                    x.reset(new ast::BinExp(x.release(), parseE2(), t));
                    break;
                }
                default:
                    return x.release();
                }
            }
        }

        ast::Exp * parseExpression() {
            std::unique_ptr<ast::Exp> x(parseE1());
            while (true) {
                switch (top().type) {
                case Token::Type::eq:
                case Token::Type::neq:
                case Token::Type::lt:
                case Token::Type::gt: {
                    ast::BinExp::Type t;
                    switch (pop().type) {
                    case Token::Type::eq:
                        t = ast::BinExp::Type::eq;
                        break;
                    case Token::Type::neq:
                        t = ast::BinExp::Type::neq;
                        break;
                    case Token::Type::lt:
                        t = ast::BinExp::Type::lt;
                        break;
                    case Token::Type::gt:
                        t = ast::BinExp::Type::gt;
                        break;
                    default:
                        assert(false and "unreachable");
                    }
                    x.reset(new ast::BinExp(x.release(), parseE1(), t));
                    break;
                }
                default:
                    return x.release();
                }
            }
        }

        ast::IfElse * parseIf() {
            pop(Token::Type::kwIf);
            pop(Token::Type::opar);
            std::unique_ptr<ast::IfElse> result(new ast::IfElse(parseExpression()));
            pop(Token::Type::cpar);
            result->ifClause = parseSequence();
            if (condPop(Token::Type::kwElse))
                result->elseClause = parseSequence();
            else
                result->elseClause = new ast::Num(0);
            return result.release();
        }

        ast::WhileLoop * parseWhile() {
            pop(Token::Type::kwWhile);
            pop(Token::Type::opar);
            std::unique_ptr<ast::WhileLoop> result(new ast::WhileLoop(parseExpression()));
            pop(Token::Type::cpar);
            result->body = parseSequence();
            return result.release();
        }

        ast::Fun * parseFunction() {
            pop(Token::Type::kwFunction);
            pop(Token::Type::opar);
            std::unique_ptr<ast::Fun> result(new ast::Fun());
            while (top() != Token::Type::cpar) {
                result->args.push_back(new ast::Var(pop(Token::Type::ident).c));
                if (not condPop(Token::Type::comma))
                    break;
            }
            pop(Token::Type::cpar);
            result->body = parseSequence();
            return result.release();
        }

        ast::Exp * parseStatement() {
            switch(top().type) {
            case Token::Type::kwIf:
                return parseIf();
            case Token::Type::kwWhile:
                return parseWhile();
            default:
                return parseExpression();
            }
        }

        void parseSequenceBody(ast::Seq * seq) {
            while (true) {
                if ((top() == Token::Type::ccbr) or (top() == Token::Type::eof))
                    return;
                seq->body.push_back(parseStatement());
            }
        }

        ast::Seq * parseSequence() {
            std::unique_ptr<ast::Seq> result(new ast::Seq());
            pop(Token::Type::ocbr);
            parseSequenceBody(result.get());
            pop(Token::Type::ccbr);
            return result.release();
        }

        Lexer l_;

    };

}

#endif
