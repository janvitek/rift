LLVM Class Exam
===============

Your task is to implement a dot product (inner product) for Rift. Dot product of two double vectors is a sum of their elements multiplied by each other, i.e.:

a = c(a1, a2, a3, a4)
b - c(b1, b2, b3, b4)

a %*% b = a1*b1 + a2*b2 + a3*b3 + a4 *b4

Note that as always in Rift, if one of the vectors is shorter, we will recycle its contents, i.e.

a = c(a1, a2, a3, a4)
b - c(b1, b2)

a %*% b = a1*b1 + a2*b2 + a3*b1 + a4 *b2

We have already updated the lexer, parser and ast nodes to accomodate the new operator. Declarations of the required functions have also been added to the files for you to fill them in. More specifically:

1) Update the compiler (in compiler.cpp) so that
