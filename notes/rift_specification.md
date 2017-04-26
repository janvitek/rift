# Rift Language Specification

Rift is a simplified language that exhibits some of the behavior of the well known dynamic array processing languages such as *R*, *MatLab* or *Julia*. This chapter gives an overview over the language features.

## Literals

The language understands the following keywords: `c`, `type`, `length`, `eval`, `if`, `else`, `while`, `function`.

    KEYWORD ::= c | type | length | eval | if | else | while | function

Identifiers start with a letter or underscore after which they may contain arbitrary number of letters, underscores or digits.

    LETTER ::= a | … | z | A | … | Z | _
    DIGIT ::= 0 | … | 9
    IDENT ::= LETTER { LETTER | DIGIT }

Number literals must being with either a digit, or with . in which case they are interpreted as fractions from interval <0;1).

    NUMBER ::= {DIGIT} . DIGIT { DIGIT}
            |= DIGIT {DIGIT} [ . DIGIT { DIGIT} ]

String literals are bound by double quotes. No escape characters are allowed and the strings may span multiple lines.

    STRING ::= “ { anything } “

Whitespace characters are a space, newline and tab.

## Grammar

    SEQ          ::= '{' { STATEMENT } '}'
    STATEMENT    ::= IF | WHILE | EXPRESSION
    EXPRESSION   ::= E1 { ( == | != | < | > ) E1 }
    E1           ::= E2 { ( + | - ) E2 }
    E2           ::= E3 { ( * | / ) E3 }
    E3           ::= F { INDEX | CALL | ASSIGNMENT }
    F            ::= NUMBER | STRING | IDENT | FUNCTION | SPECIAL_CALL | '(' EXPRESSION ')'
    CALL         ::= '(' [ EXPRESSION {, EXPRESSION } ')'
    INDEX        ::= '[' EXPRESSION ']' [ ASSIGNMENT ]
    ASSIGNMENT   ::= ( <- | = ) EXPRESSION
    SPECIAL_CALL ::= EVAL | LENGTH | TYPE | C
    EVAL         ::= eval '(' EXPRESSION ')'
    LENGTH       ::= length '(' EXPRESSION ')'
    TYPE         ::= type '(' EXPRESSION ')'
    C            ::= c '(' EXPRESSION {, EXPRESSION } ')'
    FUNCTION     ::= function '(' [ ident {, ident } ] ')' SEQ
    WHILE        ::= while '(' EXPRESSION ')' SEQ
    IF           ::= if '(' EXPRESSION ')' SEQ [ else SEQ ]

## Semantics

In the following,`>` is rift’s prompt. Lines without leading `>` are output.

Rift understands three data types - functions, double and character vectors. Character vectors are instantiated from string literals:

    > a = “foo”

(a is now character vector of length 3, elements: f, o, o). Double vectors are created from double literals, which may be concatenated together using the c function:

    > b = 4

(b is now double vector of length 1)

    > b = c(1, 2, 3)

(b is now double vector of length 3, elements 1, 2 3)

New functions are created using the function keyword, followed by the signature and function body:

    > min = function(a, b) { if (a < b) { a } else { b } }

The example above also shows a distinctive character of rift - there is no return statement, and the result of last evaluated expression is always returned from a function. Function calls are fairly non-controversial:

    > min(10, 2)
    2

All operators are vector based, if the vectors are of different length, the smaller vector will be recycled:

    > a = c(1, 2, 3, 4)
    > b = c(1, 2)
    > a + b
    2 4 4 6

One can obtain the length of a vector by using the length function and a type of the variable using the type function. Type function returns either `“double”`, `“character”`, or `“function”`:

    > a = c(1, 2)
    > length(a)
    2
    > type(a)
    double

Elements can be read and written to by the index operator `[]`. The index expression may be a vector itself, such as:

    > a = c(1, 2, 3, 4, 5, 6)
    > a[c(0, 1, 2)]
    1 2 3
    > a[c(2, 3,4)] = 10
    > a
    1 2 10 10 10 6

(note how the right hand side vector was again recycled, element indices start from 0).

The `eval` function behaves similarly to other dynamic languages - its argument (character vector) can be any valid rift source, which will be parsed and evaluated in the current
