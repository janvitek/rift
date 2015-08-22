#ifndef RUNTIME_H
#define RUNTIME_H

/** The Rift runtime manages three kinds of values, function, character
 *  vectors and double vectors. Environments are used to look up variables.
 */

struct RVal;

/** Environment */
struct Env {
  Env* parent;
  RVal lookup(char * c);
};

/** Closure */
struct Fun {
  Env *env;
  char *code; 
};


/** Character vector */
struct CV {
  int length;
  char* data;
  CV(int size) {
    data = new char[size];
  } 
  ~CV() {
    delete [] data;
  }
};

/** Double vector */
struct DV {
  int length;
  double * data;
  DV(int size) {
    data = new double[size];
  } 
  ~DV() {
    delete [] data;
  }
};

struct RVal {
  int kind;
  union u {
    Fun fun;
    CV  cv;
    DV  dv;
  };
};

DV *createDV(double size);
DV *concatDV(int size, ...);
void deleteDV(DV *v);
void setDVElem(DV *v, double index, double value);
double getDVElem(DV *v, double index);
double getDVSize(DV *v);

CV *createCV(double size);
CV *concatCV(int size, ...);
void deleteCV(CV *v);
void setCVElem(CV *v, double index, char value);
double getCVElem(CV *v, double index);
double getCVSize(CV *v);

#endif // RUNTIME_H

