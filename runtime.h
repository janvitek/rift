#ifndef RUNTIME_H
#define RUNTIME_H

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

DV * createDV(double size);
DV * concatDV(int size, ...);
void deleteDV(DV * v);
void setDVElem(DV * v, double index, double value);
double getDVElem(DV * v, double index);
double getDVSize(DV * v);

#endif // RUNTIME_H

