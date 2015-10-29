LLVM-CONFIG=llvm/bin/llvm-config

all:
	g++ `$(LLVM-CONFIG) --cxxflags --ldflags  --system-libs --libs support core mcjit native irreader linker ipo` -fexceptions -std=c++11 -o rift  -g ast.cpp compiler.cpp lexer.cpp main.cpp parser.cpp runtime.cpp

test:
	rift tests/test.ri

db:
	lldb a.out

clean:
	rm -rf *.o a.out *~ \#*  *.dSYM
