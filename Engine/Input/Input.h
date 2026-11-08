#pragma once

#include "CommonHeaders.h"
#include "EngineAPI/Input.h"

namespace mage::input {

void set(input_source::type type, input_code::code code, math::vec3 value);
}