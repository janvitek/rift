#include <assert.h>  
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

#include "lex.h"

using std::cout;
using std::endl;
using std::ifstream;
using std::ios;
using std::string;

using namespace std;

void error(const char* s) {
  cout << s << endl;  exit(-1);
}

void error(const char* s, const char* s2) {
  cout << s << s2 << endl;  exit(-1);
}


const char* tok_to_str(Token tok) {
  switch (tok) {
  case  END:    return "$"; 
  case  ASS: return "<-";
  case  NUM :   return "N";
  case  STR :   return "S";
  case  IDENT:  return "I";
  case  IF:     return "if"; 
  case  ELSE :  return "else";
  case  OPAR :  return "("; 
  case  CPAR :  return ")"; 
  case  OCBR : return "{";
  case  CCBR : return "}";
  case  OSBR : return "[";
  case  CSBR : return "]";
  case  PLUS : return "+";
  case  MIN  : return "-";
  case  DIV :  return "/"; 
  case  EQ :   return "="; 
  case  MUL:   return "*"; 
  case  FUN : return "function"; 
  case  SEP:   return ";";
  case  LT:    return "<"; 
  case  COM: return ",";
  }
}

/** True if we managed to read the input file. */
int File::good() { 
  return state == 1; 
}

/** Advance to the first non-space character.  */
void File::eat_space() {
  while ( isspace(cur_char())) advance();
  tok_start = cursor;
}

char File::cur_char() {
  return buffer[cursor];
}

char File::next_char() {
  return buffer[cursor+1];
}

int File::at_end() {
  return cursor == size;
}

void File::advance() {
  if (at_end()) error("Trying to advance past end of file");
  cursor++;
}

/** Returns the next token in the stream. */
Token File::next() {
  assert(good());
  tok = END;
  eat_space();
  if (at_end()) return tok;
  switch (cur_char()) {
  case '+': tok = PLUS; advance(); break;
  case '-': tok = MIN;  advance(); break;
  case '/': tok = DIV;  advance(); break;
  case '*': tok = MUL;  advance(); break;
  case '=': tok = EQ;   advance(); break;
  case '(': tok = OPAR; advance(); break;
  case ')': tok = CPAR; advance(); break;
  case '[': tok = OSBR; advance(); break;
  case ']': tok = CSBR; advance(); break;
  case '{': tok = OCBR; advance(); break;
  case '}': tok = CCBR; advance(); break;
  case ';': tok = SEP;  advance(); break;
  case ',': tok = COM;  advance(); break;
  case '<': 
    if (next_char() == '-') {
      tok = ASS; 
      advance();    
    } else {
      tok = LT;
    }
    advance(); break;
  case '"':
    advance();
    while( cur_char() != '"') advance();
    tok = STR;
    ident_or_string = token_as_string();
    advance();
    break;
  }
  if (tok != END) return tok;
  
  read_identifier_or_number();

  return tok;
}

string* File:: token_as_string() {
  return new string( &buffer[tok_start], cursor - tok_start);
}

void File::read_identifier_or_number() {
  int dots = 0;
  int has_letter = 0;
  int c = cur_char();
  while( isalpha(c) || isdigit(c) || c =='.' ) {
    if (c == '.') dots++;
    if (isalpha(c)) has_letter = 1;
    advance();
    c = cur_char();
  }
  int len =  cursor - tok_start;
  if (has_letter) {
    if (dots) error("Error in identifier or number");
    if ( len == 2 && strncmp( "if", &buffer[tok_start], 2) == 0 ) 
      tok = IF;
    else if ( len == 8 && strncmp( "function", &buffer[tok_start], 2) ==0 ) 
      tok = FUN;
    else if ( len == 4 && strncmp( "else", &buffer[tok_start], 2)== 0 ) 
      tok = ELSE;
    else {
      tok = IDENT;
      ident_or_string = token_as_string();
    }
  } else {
    if (dots > 1) error("Error in number");
    tok = NUM;
    string str( &buffer[tok_start], len);
    number = std::stof( str);
  }
}


/** Read in entire file 'name'.
 * If all went well, good() holds.
 */
File::File(const char* name) {
  if (name == NULL) { state = -1; return; }
  ifstream fin(name);
  if (! fin.good() ) { state = -1; return; }
  fin.seekg(0, ios::end);
  size = fin.tellg();
  buffer = new char[size];
  fin.seekg(0);
  fin.read(buffer, size); 
  fin.close();
  tok = END;
  cursor = tok_start = 0;
  state = 1; // good()
}


