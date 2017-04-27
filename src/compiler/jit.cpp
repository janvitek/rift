#include "jit.h"

namespace rift {

JIT & JIT::singleton() {
    static JIT result;
    return result;
}

}
