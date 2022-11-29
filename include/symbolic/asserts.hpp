/**
 * @file
 *   Overridable assertion/error logic used by Symbolic implementation.
 */


#ifndef SYMBOLIC_ASSERTS_HPP
#define SYMBOLIC_ASSERTS_HPP

#if !defined(SYMBL_FAIL) && !defined(SYMBL_FAIL_S)

    #include <cstdio>
    #include <cstdlib>

    #define SYMBL_FAIL(msg) \
        do { \
            std::fprintf(stderr, "failed at %s:%d: %s\n", __FILE__, __LINE__, (msg)); \
            std::abort(); \
        } while (0)

    #define SYMBL_FAIL_S() \
        do { \
            std::fprintf(stderr, "failed at %s:%d\n", __FILE__, __LINE__); \
            std::abort(); \
        } while (0)

#endif

#if !defined(SYMBL_ASSERT) && !defined(SYMBL_ASSERT_S)

    #define SYMBL_ASSERT(cond, msg) \
        do { \
            if (!(cond)) { \
                SYMBL_FAIL(msg); \
            } \
        } while (0)

    #define SYMBL_ASSERT_S(cond) \
        do { \
            if (!(cond)) { \
                SYMBL_FAIL_S(); \
            } \
        } while (0)

#endif

#if !defined(SYMBL_LOG) && !defined(SYMBL_LOGF)

    #include <cstdio>

    #define SYMBL_LOG(frmt) \
        do { \
            std::fprintf(stdout, frmt "\n"); \
        } while (0)

    #define SYMBL_LOGF(frmt, ...) \
        do { \
            std::fprintf(stdout, frmt "\n", __VA_ARGS__); \
        } while (0)

#endif

#endif  // SYMBOLIC_ASSERTS_HPP
