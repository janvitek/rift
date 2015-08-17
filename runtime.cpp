#include <cassert>
#include <cstdarg>
#include <cstring>

#include <vector>
#include "runtime.h"

DV * createIV(double size) {
    return new DV(size);
}

DV * concatIV(int size, ...) {
    va_list ap;
    va_start(ap, size);
    std::vector<DV*> inputs;
    int length = 0;
    for (int i = 0; i <= size; ++i) {
        inputs.push_back(va_arg(ap, DV*));
        length += inputs.back()->length;
    }
    va_end(ap);
    DV * result = new DV(length);
    size = 0;
    for (DV * dv : inputs) {
        memcpy(result->data + size,dv->data, dv->length);
        size += dv->length;
    }
    return result;
}

void deleteIV(DV * v) {
    delete v;
}

void setIVElem(DV * v, double index, double value) {
    assert(index >= 0 and index < v->length and "Out of bounds");
    v->data[static_cast<int>(index)] = value;
}

double getIVElem(DV * v, double index) {
    assert(index >= 0 and index < v->length and "Out of bounds");
    return v->data[static_cast<int>(index)];

}

double getIVSize(DV * v) {
    return v->length;
}
