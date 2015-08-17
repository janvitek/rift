LLVM-CONFIG=~/Downloads/llvm-3.6.2.src/Release+Asserts/bin/llvm-config

all:
	clang++  -std=c++11  -g -c lex.cpp
	clang++  `$(LLVM-CONFIG) --cxxflags --ldflags  --system-libs --libs core` -std=c++11 lex.o  -g parse.cpp

test:
	a.out test.ri

db:
	lldb a.out

clean:
	rm *.o a.out *~
