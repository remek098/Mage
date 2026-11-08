#pragma once

#include "CommonHeaders.h"

namespace mage::input {

struct axis {
    enum type : u32 {
        x = 0,
        y = 1,
        z = 2,
    };
};

struct modifier_key {
    enum key : u32 {
        none = 0x00,

        left_shift = 0x01,
        right_shift = 0x02,
        shift = left_shift | right_shift,

        left_control = 0x04,
        right_control = 0x08,
        control = left_control | right_control,

        left_alt = 0x10,
        right_alt = 0x20,
        alt = left_alt | right_alt,
    };
};

struct input_value {
    math::vec3 previous;
    math::vec3 current;
};

struct input_code {
    enum code : u32 {
        mouse_position,
        mouse_position_x,
        mouse_position_y,
        mouse_left,
        mouse_right,
        mouse_middle,
        mouse_wheel,

        key_backspace,
        key_tab,
        key_return,
        key_shift,
        key_left_shift,
        key_right_shift,
        key_control,
        key_left_control,
        key_right_control,
        key_alt,
        key_left_alt,
        key_right_alt,
        key_pause,
        key_capslock,
        key_escape,
        key_space,
        key_page_up,
        key_page_down,
        key_home,
        key_end,
        key_left,
        key_up,
        key_right,
        key_down,
        key_print_screen,
        key_insert,
        key_delete,

        key_0,
        key_1,
        key_2,
        key_3,
        key_4,
        key_5,
        key_6,
        key_7,
        key_8,
        key_9,

        key_a,
        key_b,
        key_c,
        key_d,
        key_e,
        key_f,
        key_g,
        key_h,
        key_i,
        key_j,
        key_k,
        key_l,
        key_m,
        key_n,
        key_o,
        key_p,
        key_q,
        key_r,
        key_s,
        key_t,
        key_u,
        key_v,
        key_w,
        key_x,
        key_y,
        key_z,

        key_numpad_0,
        key_numpad_1,
        key_numpad_2,
        key_numpad_3,
        key_numpad_4,
        key_numpad_5,
        key_numpad_6,
        key_numpad_7,
        key_numpad_8,
        key_numpad_9,

        key_multiply,
        key_add,
        key_subtract,
        key_decimal,
        key_divide,

        key_f1,
        key_f2,
        key_f3,
        key_f4,
        key_f5,
        key_f6,
        key_f7,
        key_f8,
        key_f9,
        key_f10,
        key_f11,
        key_f12,

        key_numlock,
        key_scrolllock
    };
};


struct input_source {
    enum type : u32 {
        keyboard,
        mouse,
        controller,
        raw,

        count
    };

    u64                     binding{ 0 };
    type                    source_type{};
    u32                     code{ 0 };
    f32                     multiplier{ 0 };
    bool                    is_discrete{ true };
    axis::type              source_axis{}; // will take input from source and map it to axis field, e.g. moving mouse in x can be mapped to rotation about y-axis.
    axis::type              axis{};
    modifier_key::key       modifier{};
};

// for polling -> copies value held by input system's internal map pointed to by the key created with type and code
// to input_value&
void get(input_source::type type, input_code::code code, input_value& value);

namespace detail {

class input_system_base {
public:
    virtual void on_event(input_source::type, input_code::code, const input_value&) = 0;

protected:
    // those will attach and detach input system
    input_system_base();
    ~input_system_base();
};

} // namespace detail

/*
doing it like this to make it aware of the type of entity scripts
so that we can register member function pointers as input handlers.

for that we need 2 pointers:
    - one that points to class instance
    - one that points to member function
*/
template<typename T>
class input_system final : public detail::input_system_base {
public:
    // NOTE: same as input_system_base::on_event signature
    using input_callback_t = void(T::*)(input_source::type, input_code::code, const input_value&);

    void add_handler(input_source::type type, T* instance, input_callback_t callback) {
        assert(instance && type < input_source::count);
        auto& collection = _input_callbacks[type];
        for (const auto& func : collection) {
            // if handler was already registered. don't add it again
            if (func.instance == instance && func.callback == callback) return;
        }

        collection.emplace_back(input_callback{ instance, callback });
    }

    void on_event(input_source::type type, input_code::code code, const input_value& value) override {
        assert(type < input_source::count);
        // call all event handlers in array that correspond to this input type
        for (const auto& item : _input_callbacks[type]) {
            (item.instance->*item.callback)(type, code, value);
        }
    }
private:
    struct input_callback {
        T*                      instance;
        input_callback_t        callback;
    };

    // NOTE: diffrent array for each type of input
    utl::vector<input_callback> _input_callbacks[input_source::count];
};

} // namespace mage::input