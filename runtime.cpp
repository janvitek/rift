#include <cassert>

#include "runtime.h"

IV * createIV(int size) {
    return new IV(size);
}

void deleteIV(IV * v) {
    delete v;
}

void setIVElem(IV * v, int index, int value) {
    assert(index >= 0 and index < v->length and "Out of bounds");
    v->data[index] = value;
}

int getIVElem(IV * v, int index) {
    assert(index >= 0 and index < v->length and "Out of bounds");
    return v->data[index];

}

int getIVSize(IV * v) {
    return v->length;
}
