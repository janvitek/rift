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

// I am using doubles in the API to spare you the need to cast them in LLVM IR, which might make more sense in the long run
DV * createIV(double size);

DV * concatIV(int size, ...);

void deleteIV(DV * v);

void setIVElem(DV * v, double index, double value);

double getIVElem(DV * v, double index);

double getIVSize(DV * v);

#endif // RUNTIME_H

