1. Run `git pull`(!)

2. fix a hole in type analysis
  * Run the example `function() {c(type(1),type("a"))}()`
  * Type returns a character vector (the name of the type). Therefore a `characterc` instruction should be emited.
    But we only see generic `c`.
  * Fix the analysis to remember that "c" returns a character (there is a TODO in `type_analysis.cpp`).

3. fix a second hole in type analysis
  * If you run the example `function(){1+2}()` doubleAdd is used.
  * If you run `function() {"a"+"b"}()` genericAdd is used however.
  * If you run with the debug flag `./rift -d` you can see that the abstract state does not know that `"a"` is a character.
  * Find out why the analysis doesn’t know that the arguments in the above example are characters (there is no TODO).
  * Fix it and make sure characterAdd is used.

4. Fix a hole in specialize
  * Run the example `function(){c(1,2,3)[1]}()` in debug mode.
  * The analysis finds out the `c(1,2,3)` is a double vector.
  * Still the vector is not accessed with the optimized runtime function `doubleGetElement`.
  * Find the TODO in specialize.cpp and implement the optimization.

5. Extend type analysis across loads and stores. (hard)
  * In the example `function() { a <- 1  a+1 }` the analysis does not know that `a` is a double. The reason is we do not track types across stores and loads.
  * Extend the analysis for `envGet` and `envSet`.
  * This is an open-ended task.
  * You will stumble across simplifying assumptions in the analysis.

