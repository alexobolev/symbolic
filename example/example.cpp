#include <cstdio>
#include <cstdlib>

#include "symbolic/names.hpp"

/// Assertion macro that works in release builds
/// and prints out successful runs.
#define check(cond) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", \
                #cond, __FILE__, __LINE__); \
            std::abort(); \
        } else { \
            std::fprintf(stdout, "Passed: %s\n", #cond); \
        } \
    } while (0)

class SystemAllocator final : public symbolic::alloc::IAllocator {
public:
    virtual ~SystemAllocator() = default;
    virtual void* allocate(std::size_t size) noexcept {
        return ::new std::uint8_t[size];
    }
    virtual void deallocate(void* data) noexcept {
        ::delete [] data;
    }
};

/// Base allocator used for reserving name arenas.
SystemAllocator g_allocator{};

namespace symbolic::detail {
    /// Name allocator which MUST be defined in user code.
    /// Does all the bump allocation, reservation, etc.
    NameAllocator g_name_allocator{ &g_allocator };
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    using namespace symbolic;

    Name joint1_a = Name::empty();
    Name joint2_a = Name::empty();
    Name ftstep_a = Name::empty();

    // First, add some names that 100% don't exist.
    // Store them for later comparison.
    {
        joint1_a = Name::add("joint_01a");
        joint2_a = Name::add("joint_02c");
        ftstep_a = Name::add("sfx/charge/heavy_footstep");

        check(joint1_a.str() == "joint_01a");
        check(joint2_a.str() == "joint_02c");
        check(ftstep_a.str() == "sfx/charge/heavy_footstep");
    }

    // Second, find the added names that 100% exist.
    // Compare them against what we added.
    {
        auto joint1_f = Name::find("joint_01a");
        auto joint2_f = Name::find("joint_02c");
        auto ftstep_f = Name::find("sfx/charge/heavy_footstep");

        check(joint1_f == joint1_a);
        check(joint2_f == joint2_a);
        check(ftstep_f == ftstep_a);

        check(joint1_f != ftstep_a);
    }

    // Third, mix and match!
    {
        auto joint1_m = Name::find_or_add("joint_01a");  // this one exists
        check(joint1_m == joint1_a);
        check(*joint1_m == "joint_01a");

        auto joint9_m = Name::find_or_add("joint_09d");  // this one doesn't
        check(*joint9_m == "joint_09d");

        auto joint9_n = Name::find_or_add("Joint_09D");  // same as above!
        check(joint9_n == joint9_m);
    }
}
