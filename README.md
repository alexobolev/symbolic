symbolic
========

A small C++ library that provides a small symbolic case-insensitive string type
for 64-bit platforms, with fast lookup and instantaneous comparison
(a single 64-bit equality check).

Inspired by Unreal Engine's `FName` system and experience reverse-engineering
BioWare's in-house optimization of it used in **Mass Effect Legendary Edition**.

[![Build Status][actions-badge]][actions-link]
[![СС0-1.0][license-badge]][license-link]

[actions-badge]: https://github.com/alexobolev/symbolic/actions/workflows/build.yml/badge.svg
[actions-link]: https://github.com/alexobolev/symbolic/actions?query=workflow%3Abuild+branch%3Amain
[license-badge]: https://img.shields.io/badge/license-CC0-blue
[license-link]: LICENSE

## Limitations

* Requires 64-bit platform.
* Only supports ASCII strings.
* Strings must be under 512 characters.

## Usage

Check the source-code in and link your library/executable to it via CMake.

If not using CMake, you need to compile `src/names.cpp` and have the `include/`
directory available for header lookup.

This library is released into public domain.

```cpp

#include "symbolic/names.hpp"

// First, define an underlying allocator type.
// This will probably use some pre-existing allocator from your codebase.
// It doesn't need to be too fast, as it would only allocate large arena "pages".

class FooAllocator : public symbolic::alloc::IAllocator {
public:
    virtual ~FooAllocator();
    virtual void* allocate(std::size_t size) noexcept {
        // ~ snip ~
    }
    virtual void deallocate(void* data) noexcept {
        // ~ snip ~
    }
};

FooAllocator g_allocator{};

// Second, define the global name allocator instance.
// It will use an instance of the allocator defined above.

namespace symbolic::detail {
    NameAllocator g_name_allocator{ &g_allocator };
}

// Third, use the `Name` type! It's very fast to copy (only 8 bytes)!

using namespace symbolic;

void foo() {
    auto joint_name = Name::find_or_add("joint_01a");

    std::string_view joint_str = *joint_name;  // = joint_name.c_str()
    assert(joint_name == Name::find("JOINT_01A"));  // it's case-insensitive
}

```
