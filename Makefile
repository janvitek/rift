LLVM-CONFIG=~/Downloads/llvm-3.6.2.src/Release+Asserts/bin/llvm-config

all:
	clang++  `$(LLVM-CONFIG) --cxxflags --ldflags  --system-libs --libs core` -std=c++11 -o rift  -g lex.cpp parse.cpp 

test:
	rift tests/test.ri

db:
	lldb a.out

clean:
	rm -rf *.o a.out *~ \#*  *.dSYM
