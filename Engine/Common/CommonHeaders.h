#pragma once

// C/Cpp headers

#include <stdint.h>
#include <assert.h>
#include <typeinfo>
//#include <limits>
#include <memory>
#include <unordered_map>

#include <string>

#if defined(_WIN64)
#include <DirectXMath.h>

// #include <atlsafe.h> // for LPSAFEARRAY stuff
#endif

// common headers
#include "PrimitiveTypes.h"
#include"..\Utilities\Math.h"
#include "../Utilities/Utilities.h"
#include "../Utilities/MathTypes.h"
#include "Id.h"

#ifdef _DEBUG
// works kinda same as assert()
#define DEBUG_ONLY_EXPR(x) x
#else
// works kinda same as assert()
#define DEBUG_ONLY_EXPR(x) (void(0))
#endif