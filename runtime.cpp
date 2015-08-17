#include <cassert>
#include <cstdarg>
#include <cstring>

#include <vector>
#include "runtime.h"

IV * createIV(int size) {
    return new IV(size);
}

IV * concatIV(int size, ...) {
    va_list ap;
    va_start(ap, size);
    std::vector<IV*> inputs;
    int length = 0;
    for (int i = 0; i <= size; ++i) {
        inputs.push_back(va_arg(ap, IV*));
        length += inputs.back()->length;
    }
    va_end(ap);
    IV * result = new IV(length);
    size = 0;
    for (IV * iv : inputs) {
        memcpy(result->data + size,iv->data, iv->length);
        size += iv->length;
    }
    return result;
}


void deleteIntVector(IV * v) {
    delete v;
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
