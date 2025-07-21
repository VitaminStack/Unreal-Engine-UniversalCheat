#pragma once

#include "../MinHook/MinHook.h"
#include "Logger.h"

#define MH_CHECK(expr) \
    do { \
        MH_STATUS _mh_status = (expr); \
        if (_mh_status != MH_OK && _mh_status != MH_ERROR_ALREADY_CREATED && _mh_status != MH_ERROR_ALREADY_INITIALIZED) { \
            LOG_ERROR("%s failed: %s", #expr, MH_StatusToString(_mh_status)); \
        } else { \
            LOG_DEBUG("%s -> %s", #expr, MH_StatusToString(_mh_status)); \
        } \
    } while (0)
