1. Run `git pull`
2. Fix the holes in runtime.cpp
  * genericSub, doubleSub
     * tests: `1-2`, `“a”-”a”`, `c(1,2,3)-1`
  * doubleGetElement
     * tests `c(1,2)[1]`, `c(1,2,3)[c(1,2)]`
3. Fix the holes in compiler.cpp
  * implement visit(ast::SimpleAssignment * n)
    * tests:
      `a <- 1`
  * implement visit(ast::UserCall * n)
    * tests:
      `(function(){1})()`
      `(function(a){a})(1)`
    * see runtime.cpp::call to figure out the rift ABI.
  * implement visit(ast::IfElse * n)
    * tests
      `(function(){if(1){1}else{2}})()`
      `(function(){if(1){if(1){1}else{2}}else{2}})()`
    * The function has to return something.
      That’s why the phi node for the result is there.
      Implement the condition checking, true/false bodies.

There are tests in tests.cpp that you should be able to enable
after implementing those items.

