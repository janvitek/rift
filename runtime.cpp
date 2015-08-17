#include <cassert>
#include <cstdarg>
#include <cstring>

#include <vector>
#include "runtime.h"

IntVector * createIntVector(int size) {
    return new IntVector(size);
}

IntVector * concatIintVector(int size, ...) {
    va_list ap;
    va_start(ap, size);
    std::vector<IntVector*> inputs;
    int length = 0;
    for (int i = 0; i <= size; ++i) {
        inputs.push_back(va_arg(ap, IntVector*));
        length += inputs.back()->length;
    }
    va_end(ap);
    IntVector * result = new IntVector(length);
    size = 0;
    for (IntVector * iv : inputs) {
        memcpy(result->data + size,iv->data, iv->length);
        size += iv->length;
    }
    return result;
}


void deleteIntVector(IntVector * v) {
    delete v;
}



void setIntVectorElement(IntVector * v, int index, int value) {
    assert(index >= 0 and index < v->length and "Out of bounds");
    v->data[index] = value;
}

int getIntVectorElement(IntVector * v, int index) {
    assert(index >= 0 and index < v->length and "Out of bounds");
    return v->data[index];

}

int getIntVectorSize(IntVector * v) {
    return v->length;
}
