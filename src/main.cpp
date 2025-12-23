#include "../shared/hooks.hpp"

// Dummy function with C linkage to avoid name mangling
extern "C" void DummyFunction() {
    // intentionally empty
}

MAKE_DLOPEN_HOOK(Test, reinterpret_cast<void*>(&DummyFunction), void) {
    // This will absolutely crash. It's just a test to make sure the compiler sees the logger
}
