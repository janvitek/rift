# Rift has the minimal set of data types need for our examples (3 types)

# Doubles

    1.2
    1.0 + 1

# Strings

    "hi"
    "hi " + "friend"


# Variables are defined so:

    v <- 1
    s <- "s"

# Assignment is an expression

    v <- w <- 2
    v + w

# Types do not mix, and we don't have casts

    1 + "two"
    c(1, "a")

# Actually this is all a lie. We really have vectors of double and vectors of characters.

    str <- "hi friend"
    str[0]
    str[1]

# For strings + is the vector concatenation operator

    str[0] + str[1]

# But we also have c() to concatenate strings

    c(str[0], str[1])

# So how do we create vectors of< doubles?  c() !

    c(1, 2.1)
    diag <- c(1, 2, 3, 4)
    diag[2]

# And + is vector addition

    diag + diag

# We have other vector ops

    diag + diag - diag * diag / diag

# Btw every double is a vector of length 1

    v <- 123
    v[0]
    1[0]
    1[1]

# We have operations to ask the type of values and their length

    type(v)
    type(str)
    type(1)
    length(diag)
    length(1)
    length(str)

# Functions can be created as expression

    id <- function( x ) { x }
    id
    id(1)
    id("hi")

# Currently we don't support function vectors, but for regularity we should

    id[0]

# Recursion works…

# rec <- function() { rec() }

# … just wait until it runs out of stack

# Functions can do more than one thing ; and while

    countdown <- function( x ) {   x <- x - 1       countdown( x )  }
    countdown( 10 )
    countdown <- function( x ) {   if ( x > 0 ) { countdown( x - 1 ) } }
    countdown( 1000 )
    countdown( 10000 )
    countdown( 100000 )

# Implementation limitations

    countdown <- function( x ) {   while ( x > 0 ) { x <- x - 1  } }
    countdown( 100000 )

# Function are higher order 

    one <- function() { 1 }
    two <- function() { 2 }

    ho <- function(a,b) { if ( a() > b() ) { a() } else { b() } }
    ho( one, two )
    ho( two, one )

# Functions are lexically scoped

    builder <- function() {  x <- 0     function() { x[0] <- x[0] + 1 } }
    counter <- builder()
    counter()
    counter()

# Values are passed by reference

x <- c(1, 2)
add1 <- function ( v ) { v[0] <- v[0] + 1 }
add1(x)
x

# and there is eval()

eval("x <- 1")
f <- function() { z <- 42   eval("x <- z") }
f()


