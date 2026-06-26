#pragma once

#include "CommonHeaders.h"
#include <combaseapi.h>

#ifndef MAGE_ED_INTERFACE
// extern "C" to avoid name mangling in cpp
#define MAGE_ED_INTERFACE extern "C" __declspec(dllexport)
#endif