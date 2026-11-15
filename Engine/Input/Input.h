#pragma once

#include "CommonHeaders.h"
#include "EngineAPI/Input.h"

namespace mage::input {

void bind(input_source source);
void unbind(input_source::type type, input_code::code code);
void unbind(u64 binding);
void set(input_source::type type, input_code::code code, math::vec3 value);
}