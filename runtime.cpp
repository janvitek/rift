#include <cassert>

#include "runtime.h"

IntVector * createIntVector(int size) {
    return new IntVector(size);
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

int getVectorSize(IntVector * v) {
    return v->length;
}

