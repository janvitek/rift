#ifndef RUNTIME_H
#define RUNTIME_H

struct IntVector {
    int length;
    int * data;
    IntVector(int size) {
        data = new int[size];
    }

    ~IntVector() {
        delete [] data;
    }
};

IntVector * createIntVector(int size);

void deleteIntVector(IntVector * v);

void setIntVectorElement(IntVector * v, int index, int value);

int getIntVectorElement(IntVector * v, int index);

int getVectorSize(IntVector * v);

#endif // RUNTIME_H

