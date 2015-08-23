LLVM-CONFIG=~/Downloads/llvm-3.6.2.src/Release+Asserts/bin/llvm-config

all:
	clang++  `$(LLVM-CONFIG) --cxxflags --ldflags  --system-libs --libs support core mcjit native irreader linker ipo` -std=c++11 -o rift  -g lex.cpp runtime.cpp parse.cpp compiler.cpp main.cpp

test:
	rift tests/test.ri

db:
	lldb a.out

clean:
	rm -rf *.o a.out *~ \#*  *.dSYM
