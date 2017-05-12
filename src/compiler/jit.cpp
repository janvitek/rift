#include "jit.h"

namespace rift {

JIT::ModuleHandle JIT::lastModule_;
bool JIT::lastModuleDeletable_;

JIT & JIT::singleton() {
    static JIT result;
    return result;
}

}
