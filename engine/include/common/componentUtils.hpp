#pragma once

// ensures this execution path only occurs once
#define ensureExecutesOnce()                     \
    {                                     \
        static bool s_isFirstTime = true; \
        if (!s_isFirstTime)               \
            return;                       \
        s_isFirstTime = false;            \
    }