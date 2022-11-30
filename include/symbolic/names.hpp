/**
 * @file
 *   Main include of the 'Symbolic' library.
 *   Exposes the name struct and its provider.
 */

#ifndef SYMBOLIC_NAMES_HPP
#define SYMBOLIC_NAMES_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string_view>

#include "symbolic/alloc.hpp"
#include "symbolic/asserts.hpp"

namespace symbolic {

    class Name;

    namespace detail {

#pragma pack( push, 1 )
#pragma pack( 1 )

        struct NameChunk final {
            static constexpr std::size_t MAX_LENGTH = 512;  // 2 ^ 9
            static constexpr std::size_t MAX_ARENAS = 65536;  // 2 ^ 16

            NameChunk*      mp_hash_next;           //< Linked list of chunks with the same (compressed) hash.
            std::uint32_t   m_hash;                 //< Hash value (uncompressed) used for searching a bucket.
            std::uint32_t   m_length : 9;           //< Number of chars in this chunk.
            std::uint32_t   m_arena_id : 16;        //< Index of the arena in which this chunk is allocated.
            std::uint32_t   m_flags : 7;            // Currently unused: extra flags.
            char            m_data[MAX_LENGTH];     // Null-terminated ASCII data - not necessarily MAX_LENGTH chars long.
        };

#pragma pack( pop )

        class NameAllocator final {
        public:

            static constexpr std::size_t MAX_ARENA_SIZE = 1024 * 1024 * 2;  // 2 MB
            static constexpr std::size_t MAX_ARENA_COUNT = 32;  // (x MAX_ARENA_SIZE) = max. memory allocated for all symbolic names

            static_assert(MAX_ARENA_COUNT <= NameChunk::MAX_ARENAS);
            static constexpr std::size_t HASH_BUCKETS = 0x10000;

        private:

            alloc::IAllocator*  mp_alloc;           //< Allocator instance that does the actual allocation.
            std::mutex          m_ctor_mtx;         //< Mutex guarding name construction.
            std::size_t         m_arena_count;      //< Number of arenas allocated.
            std::size_t         m_arena_fill;       //< Number of bytes used up in the current arena.

            std::uint8_t*       mp_arenas[MAX_ARENA_COUNT] = { nullptr };
            NameChunk*          mp_buckets[HASH_BUCKETS] = { nullptr };

        public:

            friend ::symbolic::Name;

            NameAllocator(alloc::IAllocator* allocator);
            ~NameAllocator();

        private:

            bool do_allocate_arena();
            void do_deallocate_arenas();

            NameChunk* add_name(std::string_view str, std::uint32_t hash);
        };

        /// Global instance that allocates all Names within itself.
        extern NameAllocator    g_name_allocator;

    }

#pragma pack( push, 1 )
#pragma pack( 1 )

    /// Symbolic case-insensitive ASCII name which is as cheap to copy and
    /// compare for equality as an 8-byte integer.
    ///
    /// Fairly cheap to allocate, too, since it uses a global bump allocator,
    /// though it's still better to pre-allocate names before active use, as all
    /// allocations are guarded by a mutex. I might consider something more efficient
    /// in the future.
    ///
    /// Construct using the static method 'find_or_add()'. If you are confident that
    /// the name exist - or doesn't exist - you can use simpler 'add()' and 'find()'.
    class Name final {
        std::uint64_t m_offset : 56;  //< Offset into an arena to a 'NameChunk' with this name.
        std::uint64_t m_arena_index : 8;  //< Numeric index of the arena 'm_offset' points into.

        Name() = default;
        Name(std::uint64_t offset, std::uint64_t arena);
        Name(detail::NameChunk const* chunk);

        inline detail::NameChunk const& get_entry() const {
            auto const* arena = detail::g_name_allocator.mp_arenas[m_arena_index];
            auto const* entry = reinterpret_cast<detail::NameChunk const*>(arena + m_offset);
            return *entry;
        }

    public:

        friend detail::NameAllocator;
        static constexpr std::size_t MAX_LENGTH = detail::NameChunk::MAX_LENGTH;

        //* Accessor methods and operators.
        //* ========================================

        inline std::uint32_t hash() const { return get_entry().m_hash; }
        inline std::uint32_t length() const { return get_entry().m_length; }
        inline std::uint32_t flags() const { return get_entry().m_flags; }
        inline char const*   data() const { return &(get_entry().m_data[0]); }

        inline std::string_view str() const {
            auto const& entry = get_entry();
            return { &(entry.m_data[0]), entry.m_length };
        }

        /// Shorter alias to 'Name::str()'.
        inline std::string_view operator*() const {
            return str();
        }

        /// Compare with another 'Name' in a single integer comparison.
        inline bool operator==(Name const other) const {
            // being exactly 8 bytes wide is an important constraint,
            // which we can use to reduce the number of comparisons
            return *reinterpret_cast<std::uint64_t const*>(this)
                == *reinterpret_cast<std::uint64_t const*>(&other);
        }
        inline bool operator!=(Name const other) const {
            return !(*this == other);
        }

        //* Static constructors.
        //* ========================================

        /// Get a null-initialized name (use with extreme caution).
        static Name empty();

        /// @brief Register a new name, invoking an assertion if it exists.
        /// @param str - ascii string to symbolize.
        static Name add(std::string_view str);

        /// @brief Find a name by its contents, invoking an assertion if it doesn't exist.
        /// @param str - ascii string to search for.
        static Name find(std::string_view str);

        /// @brief Find an existing name, or register a new one if it doesn't exist.
        /// @param str - ascii string to find or symbolize.
        static Name find_or_add(std::string_view str);
    };

    static_assert(sizeof(Name) == sizeof(std::uint64_t));

#pragma pack( pop )
}

#endif  // SYMBOLIC_NAMES_HPP
