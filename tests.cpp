#include "runtime.h"
#include "tests.h"
#include "type_analysis.h"

using namespace std;
using namespace rift;
namespace rift {

#define TEST(code, ...) test(__LINE__, code, new RVal({__VA_ARGS__}))
#define HAS_RUNTIME_CALL(code, name) testFunctionContains(__LINE__, code, name)
#define HAS_NOT_RUNTIME_CALL(code, name) testFunctionNotContains(__LINE__, code, name)

    void test(int line, char const * source, RVal * expected) {
        try {
            Environment * env = new Environment(nullptr);
            RVal * actual = eval(env, source);
            if (*expected != *actual) {
                cout << "Expected: " << *expected << endl;
                cout << "Actual: " << *actual << endl;
                throw "Expected and actual results do not match";
            }
        } catch (char const * e) {
            cout << "ERROR at line " << line << " : " << e << endl;
            cout << source << endl << endl;
        } catch (...) {
            cout << "ERROR at line " << line << " : " << endl;
            cout << source << endl << endl;

        }
    }

    bool containsRuntimeCall(llvm::Function * f, char const * name) {
        for (auto & b : *f) {
            for (auto & i : b) {
                if (llvm::CallInst * ci = llvm::dyn_cast<llvm::CallInst>(&i)) {
                    llvm::StringRef s = ci->getCalledFunction()->getName();
                    if (s == name)
                        return true;
                }
            }
        }
        return false;
    }

    void testFunctionContains(int line, char const * source, char const * name) {
        try {
            Environment * env = new Environment(nullptr);
            RVal * actual = eval(env, source);
            if (not containsRuntimeCall(actual->f->bitcode, name))
                throw "Runtime call not found";
        } catch (char const * e) {
            cout << "ERROR at line " << line << " : " << e << endl;
            cout << source << endl << endl;
        } catch (...) {
            cout << "ERROR at line " << line << " : " << endl;
            cout << source << endl << endl;
        }
    }

    void testFunctionNotContains(int line, char const * source, char const * name) {
        try {
            Environment * env = new Environment(nullptr);
            RVal * actual = eval(env, source);
            if (containsRuntimeCall(actual->f->bitcode, name))
                throw "Runtime call found";
        } catch (char const * e) {
            cout << "ERROR at line " << line << " : " << e << endl;
            cout << source << endl << endl;
        } catch (...) {
            cout << "ERROR at line " << line << " : " << endl;
            cout << source << endl << endl;
        }
    }


    /** Passing this means you emit proper runtime call for the generic dot operator.
     */
    void project2() {
        HAS_RUNTIME_CALL("function(a, b) { a %*% b }", "genericDot");
    }


    /** These tests check that the generic runtime function for the dot product works properly.
     */
    void project3() {
        TEST("(f = function(a, b) { a %*% b })(c(1,2,3), c(3,2,1))", 10);
        TEST("(f = function(a, b) { a %*% b })(c(1,2,3), 3)", 18);
        TEST("(f = function(a, b) { a %*% b })(10, 2)", 20);
        TEST("(f = function(a, b) { a %*% b })(c(1,2,3,4), c(5,6))", 5 + 6 * 2 + 3 * 5 + 4 * 6);
    }

    /** Checks that a dot operator result type is correctly set to be double scalar by the type analysis. */
    void project4() {
        char const * source = "function(a, b) { a %*% b }";
        try {
            Environment * env = new Environment(nullptr);
            RVal * actual = eval(env, source);
            llvm::Function * f = actual->f->bitcode;
            // run type analysis on the function
            //auto pm = llvm::legacy::FunctionPassManager(f->getParent());
            TypeAnalysis ta;
            ta.runOnFunction(*f);
            // check the type of the genericDot intrinsic call
            for (auto & b : *f) {
                for (auto & i : b) {
                    if (llvm::CallInst * ci = llvm::dyn_cast<llvm::CallInst>(&i)) {
                        llvm::StringRef s = ci->getCalledFunction()->getName();
                        if (s == "genericDot") {
                            AType * t = ta.state.get(ci);
                            if (t->kind != AType::Kind::R or not t->isScalar())
                                throw "Generic dot does not have correct type";
                            if (ta.state.getLocation(t) != ci)
                                throw "Location attached to the generic dot type is invalid";
                            t = t->payload;
                            if (ta.state.getLocation(t) != nullptr)
                                throw "Generic dot payload cannot have location";
                            t = t->payload;
                            if (ta.state.getLocation(t) != nullptr)
                                throw "Generic dot payload cannot have location";
                            return;
                        }
                    }
                }
            }
            throw "Generic dot not found";
        } catch (char const * e) {
            cout << "ERROR at line " << __LINE__ << " : " << e << endl;
            cout << source << endl << endl;
        } catch (...) {
            cout << "ERROR at line " << __LINE__ << " : " << endl;
            cout << source << endl << endl;
        }
    }

    /** Tests that the double vector specialization is properly inserted by the optimization and that it performs
        properly.
     */
    void project5() {
        HAS_RUNTIME_CALL("function() { c(1,2) %*% c(3,4) }", "doubleDot");
        HAS_RUNTIME_CALL("function() { c(1,2, 3) %*% 5 }", "doubleDot");
        HAS_NOT_RUNTIME_CALL("function() { c(1,2) %*% c(3,4) }", "genericDot");
        HAS_NOT_RUNTIME_CALL("function() { c(1,2, 3) %*% 5 }", "genericDot");
        TEST("c(1,2) %*% c(3,4)", 11);
        TEST("c(1,2) %*% 3", 9);
    }

    /** Tests that the double scalar specialization works and that double or generic dot calls are not emitted.
     */
    void project6() {
        HAS_NOT_RUNTIME_CALL("function() { 3 %*% 4 }", "genericDot");
        HAS_NOT_RUNTIME_CALL("function() { 3 %*% 4 }", "doubleDot");
        TEST("3 %*% 4", 12);
    }

    void tests() {
        cout << "Running tests..." << endl;
        TEST("1", 1);
        TEST("1 + 2", 3);
        TEST("1 - 2", -1);
        TEST("2 * 3", 6);
        TEST("10 / 5", 2);
        TEST("2 == 2", 1);
        TEST("2 == 3", 0);
        TEST("2 != 3", 1);
        TEST("2 != 2", 0);
        TEST("1 < 2", 1);
        TEST("2 < 1", 0);
        TEST("3 > 1", 1);
        TEST("3 > 10", 0);
        TEST("c(1,2)", 1, 2);
        TEST("c(1, 2, 3)", 1, 2, 3);
        TEST("c(1, 2) + c(3, 4)", 4, 6);
        TEST("c(1, 2) - c(2, 1)", -1, 1);
        TEST("c(2, 3) * c(3, 4)", 6, 12);
        TEST("c(10, 9) / c(2, 3)", 5, 3);
        TEST("c(1,2) == c(3, 4)", 0, 0);
        TEST("c(1,2) == c(1, 4)", 1, 0);
        TEST("c(1,2) == c(3, 2)", 0, 1);
        TEST("c(1,2) == c(1, 2)", 1, 1);
        TEST("c(1,2) != c(3, 4)", 1, 1);
        TEST("c(1,2) != c(1, 4)", 0, 1);
        TEST("c(1,2) != c(3, 2)", 1, 0);
        TEST("c(1,2) != c(1, 2)", 0, 0);
        TEST("c(1,2) < c(3,4)", 1, 1);
        TEST("c(1,2) < c(2, 1)", 1, 0);
        TEST("c(3,4) > c(5, 6)", 0, 0);
        TEST("c(1,2,3) < 2", 1, 0, 0);
        TEST("c(1,2,3) + c(1,2)", 2, 4, 4);

        TEST("\"a\"", "a");
        TEST("\"foo\" + \"bar\"", "foobar");
        TEST("\"aba\" == \"aca\"", 1, 0, 1);
        TEST("\"aba\" == c(1,2)", 0);
        TEST("\"aba\" != c(1,2)", 1);

        TEST("a = 1 a", 1);
        TEST("a = 1 a = a + 2 a", 3);
        TEST("a = 1 a = a - a a", 0);
        TEST("a = 2 a = a * 3 a", 6);
        TEST("a = 20 a / 4", 5);
        TEST("a = 1 b = 2 a < b", 1);
        TEST("a = 1 a = c(a, a)", 1, 1);

        TEST("type(1)", "double");
        TEST("type(\"a\")", "character");
        TEST("type(function() { 1 })", "function");
        TEST("length(1)", 1);
        TEST("length(\"aba\")", 3);
        TEST("length(\"\")", 0);

        TEST("a = 1 if (a) { 1 } else { 2 }", 1);
        TEST("a = 1 b = 1 if (a) { b = 2 } b", 2);
        TEST("a = 0 b = 1 if (a) { b = 2 } b", 1);
        TEST("a = 10 b = 0 while (a > 0) { b = b + 1 a = a - 1 } c(a, b)", 0, 10);

        TEST("f = function() { 1 } f()", 1);
        TEST("f = function(a, b) { a + b } f(1, 2)", 3);
        TEST("f = function() { a + b } a = 1 b = 2 f()", 3);
        TEST("f = function() { a = 1 a } a = 2 c(f(), a)", 1, 2);

        TEST("a = c(1, 2, 3) a[1]", 2);
        TEST("a = \"aba\" a[c(0,2)]", "aa");
        TEST("a = c(1,2,3) a[c(0,1)] = 56 a", 56, 56, 3);

        // project2();
        // project3();
        // project4();
        // project5();
        // project6();
    }

} // namespace rift
