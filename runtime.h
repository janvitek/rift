#ifndef RUNTIME_H
#define RUNTIME_H

struct IV {
    int length;
    int * data;
    IV(int size) {
        data = new int[size];
    }

    ~IV() {
        delete [] data;
    }
};

IV * createIV(int size);

void deleteIV(IV * v);

void setIVElem(IV * v, int index, int value);

int getIVElem(IV * v, int index);

int getIVSize(IV * v);

#endif // RUNTIME_H

