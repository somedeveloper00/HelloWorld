namespace engine
{
#include <stdlib.h>

// set environment variable
static inline void setEnvironment(const char *name, const char *value)
{
#if defined(_WIN32) || defined(_WIN64)
    _putenv_s(name, value);
#elif defined(__linux__)
    setenv(name, value, 1)
#else
#error "platform not supported"
#endif
}
} // namespace engine