/**
 * @file
 *   Allocator types used by the 'Symbolic' library.
 */

#ifndef SYMBOLIC_ALLOC_HPP
#define SYMBOLIC_ALLOC_HPP

#include <cstddef>
#include <cstdint>

namespace symbolic::alloc {

    /// Underlying allocator interface.
    ///
    /// All Symbolic allocations will go through
    /// an instance of an 'IAllocator' implementation.
    class IAllocator {
    public:
        ~IAllocator() = default;
        virtual void* allocate(std::size_t size) noexcept = 0;
        virtual void deallocate(void* data) noexcept = 0;
    };
}

#endif  // SYMBOLIC_ALLOC_HPP
