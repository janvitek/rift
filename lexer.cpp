#include <assert.h>  
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

#include "lexer.h"
#include "pool.h"

namespace rift {

Token Lexer::identOrKeyword(char start, std::istream & input) {
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
        return Token(Token::Type::ident, Pool::addToPool(x));
}

Token Lexer::stringLiteral(std::istream & input) {
    std::string x;
    while (input.peek() != '"')
        x += input.get();
    input.get(); // closing "
    return Token(Token::Type::character, Pool::addToPool(x));
}

}

