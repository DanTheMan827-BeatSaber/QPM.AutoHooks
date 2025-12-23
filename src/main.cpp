#include "../shared/hooks.hpp"

extern "C" void DummyFunction() {}

constexpr void* DummyAddr = reinterpret_cast<void*>(&DummyFunction);

MAKE_DLOPEN_HOOK(Test, DummyAddr, void) {
    // This will absolutely crash. It's just a test to make sure the compiler sees the logger
}
