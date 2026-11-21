#include "Components/Entity.h"
#include "Components/Transform.h"
#include "Components/Script.h"
#include "EngineAPI/Input.h"

using namespace mage;

class rotator_script;
REGISTER_SCRIPT(rotator_script);
class rotator_script : public script::script_entity {
public:
    constexpr explicit rotator_script(mage::game_entity::entity entity)
        : mage::script::script_entity(entity) {}

    virtual void begin_play() {}
    virtual void update(f32 dt) {
        _angle += 0.25f * dt * math::two_pi;
        if (_angle > -math::two_pi) _angle -= math::two_pi;
        math::vec3a rot{ 0.f, _angle, 0.f };
        DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot)) };
        math::vec4 rot_quat{};
        DirectX::XMStoreFloat4(&rot_quat, quat);
        set_rotation(rot_quat);
    }
private:
    f32 _angle{ 0.f };
};


class camera_script;
REGISTER_SCRIPT(camera_script);
class camera_script : public script::script_entity {
public:
    explicit camera_script(mage::game_entity::entity entity)
        : mage::script::script_entity(entity) 
    {
        _input_system.add_handler(input::input_source::mouse, this, &camera_script::mouse_move);

        const u64 binding{ std::hash<std::string>()("move") };
        _input_system.add_handler(binding, this, &camera_script::on_move);

        math::vec3 pos{ position() };
        _desired_position = _position = DirectX::XMLoadFloat3(&pos);

        math::vec3 dir{ orientation() };
        f32 theta{ DirectX::XMScalarACos(dir.y) };
        f32 phi{ std::atan2(-dir.z, dir.x) };
        math::vec3 rot{ theta - math::half_pi, phi + math::half_pi, 0.f };
        _desired_spherical = _spherical = DirectX::XMLoadFloat3(&rot);
    }

    virtual void begin_play() override {}
    virtual void update(f32 dt) override {
        using namespace DirectX;
        if (_move_magnitude > math::epsilon) {
            const f32 fps_scale{ dt / 0.016667f };
            math::vec4 rot{ rotation() };
            DirectX::XMVECTOR d{ XMVector3Rotate(_move * 0.08f * fps_scale, XMLoadFloat4(&rot)) };
            if (_position_acceleration < 1.f) _position_acceleration += (0.02f * fps_scale);
            _desired_position += (d * _position_acceleration);
            _b_move_position = true;
        }
        else if (_b_move_position) {
            _position_acceleration = 0.f;
        }

        if (_b_move_position || _b_move_rotation) {
            camera_seek(dt);
        }
    }
private:
    void on_move(u64 binding, const input::input_value& value) {
        using namespace DirectX;

        _move = XMLoadFloat3(&value.current);
        _move_magnitude = XMVectorGetX(XMVector3LengthSq(_move));
    }

    void mouse_move(input::input_source::type type, input::input_code::code code, const input::input_value& mouse_pos) {
        if (code == input::input_code::mouse_position) {
            input::input_value value;
            input::get(input::input_source::mouse, input::input_code::mouse_left, value);
            if (value.current.z == 0.f) return; // meaning the left button has not been pressed.

            const f32 scale{ 0.002f };
            const f32 dx{ (mouse_pos.current.x - mouse_pos.previous.x) * scale };
            const f32 dy{ (mouse_pos.current.y - mouse_pos.previous.y) * scale };

            math::vec3 spherical;
            DirectX::XMStoreFloat3(&spherical, _desired_spherical);
            // NOTE: x and y directions are switched because moving the mouse horizontally (in x direction) results in 
            // rotation about y axis (which is the up-vector)
            // same way moving vertically results in the camera tilting around it's x axis 
            spherical.x += dy;
            spherical.y -= dx;
            // clamping around almost straight up and almost straight down
            spherical.x = math::clamp(spherical.x, 0.0001f - math::half_pi, math::half_pi - 0.0001f);

            _desired_spherical = DirectX::XMLoadFloat3(&spherical);
            _b_move_rotation = true;
        }
    }

    void camera_seek(f32 dt) {
        using namespace DirectX;
        XMVECTOR p{ _desired_position - _position };
        XMVECTOR o{ _desired_spherical - _spherical };

        _b_move_position = XMVectorGetX(XMVector3Length(p)) > math::epsilon;
        _b_move_rotation = XMVectorGetX(XMVector3Length(o)) > math::epsilon;

        const f32 scale{ 0.2f * dt / 0.016667f };

        if (_b_move_position) {
            _position += p * scale;
            math::vec3 new_pos;
            XMStoreFloat3(&new_pos, _position);
            set_position(new_pos);
        }

        if (_b_move_rotation) {
            _spherical += o * scale;
            math::vec3 new_rot;
            XMStoreFloat3(&new_rot, _spherical);
            new_rot.x = math::clamp(new_rot.x, 0.0001f - math::half_pi, math::half_pi - 0.0001f);
            _spherical = DirectX::XMLoadFloat3(&new_rot);

            DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(_spherical) };
            math::vec4 rot_quat;
            DirectX::XMStoreFloat4(&rot_quat, quat);
            set_rotation(rot_quat);
        }
    }


private:
    input::input_system<camera_script>  _input_system{};

    DirectX::XMVECTOR                   _desired_position;
    DirectX::XMVECTOR                   _desired_spherical;
    DirectX::XMVECTOR                   _position;
    DirectX::XMVECTOR                   _spherical;
    DirectX::XMVECTOR                   _move{};
    f32                                 _move_magnitude{ 0.f };
    f32                                 _position_acceleration{ 0.f };
    bool                                _b_move_position{ false };
    bool                                _b_move_rotation{ false };
};