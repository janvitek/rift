#ifndef H_LEX
#define H_LEX

#include <llvm/IR/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Analysis/Passes.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"


enum Token {
  END = -1,
  ASS = 1,
  NUM = 2,
  STR = 3,
  IDENT = 4,
  IF = 5,
  ELSE = 6,
  OPAR = 7,
  CPAR = 8,
  OCBR = 9,
  CCBR = 10,
  OSBR = 11,
  CSBR = 12,
  PLUS = 14,
  MIN = 15,
  DIV = 16,
  EQ = 17,
  MUL = 18,
  FUN = 19,
  SEP = 20,
  LT = 21,
  COM = 22,
};

char const* tok_to_str(Token tok);
void error(char const* s);
void error(char const * s, char const* s2);

class File  {
  int state;
  char* buffer;
  size_t size;
  Token tok;
  int cursor;
  int tok_start;
  void eat_space();
  char cur_char();
  int at_end();
  char next_char();
  void advance();
  void read_identifier_or_number();

public:
  File(char const* name);
  bool good();
  Token next();
  std::string* token_as_string();
  std::string* ident_or_string;
  float number;
};

#endif
