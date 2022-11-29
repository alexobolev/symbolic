#include "symbolic/names.hpp"

#include <algorithm>

namespace symbolic::utils {
    /// Check if a character is a valid ASCII value.
    constexpr bool is_ascii(char const ch) noexcept {
        return ch >= '\0' && ch <= '\x7F';
    }

    /// Check if a string is entirely comprised of ASCII values.
    constexpr bool is_fully_ascii(std::string_view const str) noexcept {
        return std::all_of(str.cbegin(), str.cend(),
            [] (char const ch) { return is_ascii(ch); });
    }

    /// Check if a character is an uppercase letter ASCII value.
    constexpr bool is_upper_alpha_ascii(char const ch) noexcept {
        return ch >= 'A' && ch <= 'Z';
    }

    /// Transform an ASCII character to lowercase.
    constexpr char to_lower_ascii(char const ch) noexcept {
        if (is_upper_alpha_ascii(ch))
            return ch + ('a' - 'A');
        return ch;
    }

    /// Run a case-insensitive version of djb2 hash on a string without any extra allocations.
    constexpr std::uint32_t hash_ascii_ci(char const* const data, std::size_t const length) noexcept {
        std::uint32_t hash = 5381;
        for (auto u = 0u; u < length; ++u) {
            auto const lower = to_lower_ascii(data[u]);
            auto const ch = static_cast<std::int32_t>(lower);
            hash = ((hash << 5) + hash) ^ ch;
        }
        return hash;
    }
}

namespace symbolic::detail {
    NameAllocator::NameAllocator(alloc::IAllocator* const allocator)
        : mp_alloc{ allocator }
    {
        SYMBL_ASSERT(mp_alloc != nullptr, "base allocator can't be null");
        if (!do_allocate_arena()) {
            SYMBL_FAIL("failed to pre-allocate the initial name arena");
        }
        std::fill(&mp_buckets[0], &mp_buckets[HASH_BUCKETS], nullptr);
    }
    NameAllocator::~NameAllocator() {
        do_deallocate_arenas();
    }

    bool NameAllocator::do_allocate_arena() {
        if (m_arena_count + 1 > MAX_ARENA_COUNT) {
            SYMBL_LOG("exhausted NameChunk arenas");
            return false;
        }

        auto new_arena = reinterpret_cast<std::uint8_t*>(mp_alloc->allocate(MAX_ARENA_SIZE));
        if (new_arena == nullptr) {
            SYMBL_LOG("failed to allocate a new NameChunk arena");
            return false;
        }

        std::fill_n(new_arena, MAX_ARENA_SIZE, 0u);
        mp_arenas[m_arena_count] = new_arena;

        m_arena_count++;
        m_arena_fill = 0u;

        return true;
    }
    void NameAllocator::do_deallocate_arenas() {
        for (auto u = 0u; u < m_arena_count; ++u) {
            mp_alloc->deallocate(mp_arenas[u]);
        }
        m_arena_count = 0;
        m_arena_fill = 0;
    }

    NameChunk* NameAllocator::add_name(std::string_view const str, std::uint32_t const hash) {
        std::size_t const meta_size = offsetof(NameChunk, m_data);
        std::size_t const str_size = str.size() + 1;  // add a byte for null-terminator
        std::size_t const entry_size = meta_size + str_size;

        if (m_arena_fill + entry_size >= MAX_ARENA_SIZE) {
            if (!do_allocate_arena()) {
                SYMBL_FAIL("failed to allocate a new NameChunk arena");
                return nullptr;
            }
        }

        // instead of allocating entry on stack,
        // allocate and modify it directly within the arena

        std::uint8_t* const arena = mp_arenas[m_arena_count - 1];
        std::uint8_t* const new_alloc = arena + m_arena_fill;

        NameChunk* const new_chunk = reinterpret_cast<NameChunk* const>(new_alloc);

        new_chunk->mp_hash_next = nullptr;
        new_chunk->m_hash = hash;
        new_chunk->m_length = str.size();
        new_chunk->m_arena_id = m_arena_count - 1;
        new_chunk->m_flags = 0;

        std::copy_n((std::uint8_t*)(str.data()), str_size, (std::uint8_t*)new_chunk + meta_size);
        m_arena_fill += entry_size;

        return new_chunk;
    }
}

namespace symbolic {
    Name::Name(std::uint64_t const offset, std::uint64_t const arena)
        : m_offset{ offset }, m_arena_index{ arena } {}

    Name::Name(detail::NameChunk const* chunk) {
        using namespace detail;

        m_arena_index = chunk->m_arena_id;
        SYMBL_ASSERT_S(m_arena_index <= g_name_allocator.m_arena_count);

        auto const entry_address = reinterpret_cast<std::uint8_t const*>(chunk);
        auto const base_address = g_name_allocator.mp_arenas[m_arena_index];

        m_offset = static_cast<uint64_t>(entry_address - base_address);
        SYMBL_ASSERT_S(m_offset <= NameAllocator::MAX_ARENA_SIZE);
    }

    Name Name::add(std::string_view const str) {
        using namespace detail;

        SYMBL_ASSERT(utils::is_fully_ascii(str), "can't add a non-ascii name");
        SYMBL_ASSERT(str.size() < MAX_LENGTH, "can't add a name this long");

        std::uint32_t const hash = utils::hash_ascii_ci(str.data(), str.size());
        std::uint32_t const bucket = hash % NameAllocator::HASH_BUCKETS;

        std::scoped_lock const lock{ g_name_allocator.m_ctor_mtx };

        // the best case - this bucket is empty so far:
        // (note the reference to a pointer)

        NameChunk*& slot = g_name_allocator.mp_buckets[bucket];
        if (slot == nullptr) {
            slot = g_name_allocator.add_name(str, hash);
            return Name{ slot };
        }

        // the less ideal case - bucket must be iterated to the end:

        NameChunk* chain = slot;
        for (; chain->mp_hash_next != nullptr; chain = chain->mp_hash_next) {
            if (chain->m_hash == hash) {
                SYMBL_FAIL("this name has already been registered");
            }
        }

        chain->mp_hash_next = g_name_allocator.add_name(str, hash);
        return Name{ chain->mp_hash_next };
    }

    Name Name::find(std::string_view const str) {
        using namespace detail;

        SYMBL_ASSERT(utils::is_fully_ascii(str), "can't find a non-ascii name");
        SYMBL_ASSERT(str.size() < MAX_LENGTH, "can't find a name this long");

        std::uint32_t const hash = utils::hash_ascii_ci(str.data(), str.size());
        std::uint32_t const bucket = hash % NameAllocator::HASH_BUCKETS;

        NameChunk const* chain = g_name_allocator.mp_buckets[bucket];
        for (; chain != nullptr; chain = chain->mp_hash_next) {
            if (chain->m_hash == hash) {
                return Name{ chain };
            }
        }

        SYMBL_FAIL("failed to find this name");
    }

    Name Name::find_or_add(std::string_view const str) {
        using namespace detail;

        SYMBL_ASSERT(utils::is_fully_ascii(str), "can't find or add a non-ascii name");
        SYMBL_ASSERT(str.size() < MAX_LENGTH, "can't find or add a name this long");

        std::uint32_t const hash = utils::hash_ascii_ci(str.data(), str.size());
        std::uint32_t const bucket = hash % NameAllocator::HASH_BUCKETS;

        // try to find the name first:

        {
            NameChunk const* chain = g_name_allocator.mp_buckets[bucket];
            for (; chain != nullptr; chain = chain->mp_hash_next) {
                if (chain->m_hash == hash) {
                    return Name{ chain };
                }
            }
        }

        // if not found, add the name:

        {
            std::scoped_lock const lock{ g_name_allocator.m_ctor_mtx };

            // the best case - the bucket is empty:

            NameChunk*& slot = g_name_allocator.mp_buckets[bucket];
            if (slot == nullptr) {
                slot = g_name_allocator.add_name(str, hash);
                return Name{ slot };
            }

            // the worse case - bucket must be iterated to the end:
            // (note that hash has already been checked here, unlike in the unconditional 'add')

            NameChunk* chain = slot;
            for (; chain->mp_hash_next != nullptr; chain = chain->mp_hash_next) ;

            chain->mp_hash_next = g_name_allocator.add_name(str, hash);
            return Name{ chain->mp_hash_next };
        }
    }

}
