#ifndef H_PARSE
#define H_PARSE

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
/*
        SEQ ::= '{' SEQ_BODY '}'
        SEQ_BODY ::= {STATEMENT}
        STATEMENT ::= IF | WHILE | EXPRESSION
        EXPRESSION ::= E1 { ( == | != | <= | >=) E1 }
        E1 ::= E2 { ( + | -) E1 }
        E2 ::= E3 { ( * | / ) E2 }
        E2 ::= CALL | (VAR | INDEX) [<- EXPRESSION] | num | char | '(' EXPRESSION ')'
 */


        ast::Call * parseCall(ast::Var * variable) {
            std::unique_ptr<ast::UserCall> call(new ast::UserCall(variable));
            while (top() != Token::Type::cpar) {
                call->addArgument(parseExpression());
                if (not condPop(Token::Type::comma))
                    break;
            }
            pop(Token::Type::cpar);
            return call.release();
        }

        ast::SimpleAssignment * parseAssignment(ast::Var * variable) {
            std::unique_ptr<ast::SimpleAssignment> assign(new ast::SimpleAssignment(variable));
            assign->setRhs(parseExpression());
            return assign.release();
        }

        ast::Exp * parseIndex(ast::Var * variable) {
            std::unique_ptr<ast::Index> index(new ast::Index(variable));
            index->setIndex(parseExpression());
            pop(Token::Type::csbr);
            if (condPop(Token::Type::assign)) {
                std::unique_ptr<ast::IndexAssignment> ia(new ast::IndexAssignment(index.release()));
                ia->setRhs(parseExpression());
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
            return (new ast::SpecialCall(Token::Type::kwEval))->addArgument(arg.release());
        }

        ast::Exp * parseLength() {
            pop(Token::Type::kwLength);
            pop(Token::Type::opar);
            std::unique_ptr<ast::Exp> arg(parseExpression());
            pop(Token::Type::cpar);
            return (new ast::SpecialCall(Token::Type::kwLength))->addArgument(arg.release());
        }

        ast::Exp * parseType() {
            pop(Token::Type::kwType);
            pop(Token::Type::opar);
            std::unique_ptr<ast::Exp> arg(parseExpression());
            pop(Token::Type::cpar);
            return (new ast::SpecialCall(Token::Type::kwType))->addArgument(arg.release());
        }

        ast::Exp * parseC() {
            pop(Token::Type::kwC);
            pop(Token::Type::opar);
            std::unique_ptr<ast::SpecialCall> result(new ast::SpecialCall(Token::Type::kwC));
            do {
                result->addArgument(parseExpression());
            } while (condPop(Token::Type::comma));
            pop(Token::Type::cpar);
            return result.release();
        }

        ast::Exp * parseE3() {
            switch (top().type) {
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
                std::unique_ptr<ast::Var> v(new ast::Var(pop(Token::Type::ident).c));
                // if we see ( it's a call
                if (condPop(Token::Type::opar))
                    return parseCall(v.release());
                // if we see <- it is an assignment
                if (condPop(Token::Type::assign))
                    return parseAssignment(v.release());
                // if we see [ it is index, or later index assignment
                if (condPop(Token::Type::osbr))
                    return parseIndex(v.release());
                return v.release();
            }
        }

        ast::Exp * parseE2() {
            std::unique_ptr<ast::Exp> x(parseE3());
            while (true) {
                switch (top().type) {
                case Token::Type::mul:
                case Token::Type::div: {
                    Token::Type t = pop().type;
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
                    Token::Type t = pop().type;
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
                    Token::Type t = pop().type;
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
            result->setIfClause(parseSequence());
            if (condPop(Token::Type::kwElse))
                result->setElseClause(parseSequence());
            else
                result->setElseClause(new ast::Num(0));
            return result.release();
        }

        ast::WhileLoop * parseWhile() {
            pop(Token::Type::kwWhile);
            pop(Token::Type::opar);
            std::unique_ptr<ast::WhileLoop> result(new ast::WhileLoop(parseExpression()));
            pop(Token::Type::cpar);
            result->setBody(parseSequence());
            return result.release();
        }

        ast::Fun * parseFunction() {
            pop(Token::Type::kwFunction);
            pop(Token::Type::opar);
            std::unique_ptr<ast::Fun> result(new ast::Fun());
            while (top() != Token::Type::cpar) {
                result->addArgument(new ast::Var(pop(Token::Type::ident).c));
                if (not condPop(Token::Type::comma))
                    break;
            }
            pop(Token::Type::cpar);
            result->setBody(parseSequence());
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
                seq->push_back(parseStatement());
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
