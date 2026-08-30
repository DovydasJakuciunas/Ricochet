#include "player.hpp"
#include "aircraft.hpp"
#include <cmath>

struct AircraftMover
{
    AircraftMover(float vx, float vy) : velocity(vx, vy) {}
    void operator()(Aircraft& aircraft, sf::Time) const
    {
        aircraft.Accelerate(velocity);
    }

    sf::Vector2f velocity;
};

struct AircraftRotator
{
    AircraftRotator(float rotation) : rotation(rotation) {}
    void operator()(Aircraft& aircraft, sf::Time) const
    {
        // Only allow turning if there's forward acceleration (velocity > 0)
        sf::Vector2f velocity = aircraft.GetVelocity();
        float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

        if (speed > 0.f)
        {
            aircraft.rotate(sf::degrees(rotation));
            // Always align velocity to rotation so movement follows the nose direction
            aircraft.AlignVelocityToRotation();
            // Always update stored velocity so decelerator uses the new direction
            aircraft.StoreVelocityAtRelease();
        }
    }

    float rotation;
};

struct AircraftForwardMover
{
    void operator()(Aircraft& aircraft, sf::Time dt) const
    {
        float speed = aircraft.GetMaxSpeed();

        double radians = Utility::toRadians(aircraft.getRotation().asDegrees() + 90.f);
        float dirX = -std::cos(radians);
        float dirY = -std::sin(radians);

        sf::Vector2f currentVelocity = aircraft.GetVelocity();
        float currentSpeed = std::sqrt(currentVelocity.x * currentVelocity.x + currentVelocity.y * currentVelocity.y);

        // Increment forward acceleration time
        aircraft.IncrementForwardTime(dt);

        float holdTime = aircraft.GetForwardAccelerationTime().asSeconds();

        // Base acceleration rate
        const float accelerationRate = 300.f;
        const float boostedAccelerationRate = 15000.f;
        const float boostThreshold = 2.f;

        float deltaTime = dt.asSeconds();
        float acceleration = accelerationRate * deltaTime;

        // Apply boosted acceleration if held for more than 2 seconds
        if (holdTime > boostThreshold)
        {
            acceleration = boostedAccelerationRate * deltaTime;
        }

        if (currentSpeed < speed)
        {
            float newSpeed = std::min(currentSpeed + acceleration, speed);
            aircraft.SetVelocity(newSpeed * dirX, newSpeed * dirY);
        }
        else
        {
            aircraft.SetVelocity(speed * dirX, speed * dirY);
        }
    }
};

struct AircraftForwardAccelerationReset
{
    void operator()(Aircraft& aircraft, sf::Time) const
    {
        aircraft.ResetForwardTime();
        aircraft.ResetReleaseTime();
        aircraft.StoreVelocityAtRelease();
    }
};

struct AircraftDecelerator
{
    void operator()(Aircraft& aircraft, sf::Time dt) const
    {
        aircraft.IncrementReleaseTime(dt);

        float releaseTime = aircraft.GetReleaseTime().asSeconds();

        // Deceleration: starts immediately after release, completes stop by 3 seconds (0.5s delay + 2.5s linear decel)
        if (releaseTime >= 0.5f && releaseTime < 3.0f)
        {
            // Linearly decelerate over 2.5 seconds (from 0.5 to 3.0) based on initial velocity at release
            float decelerationProgress = (releaseTime - 0.5f) / 2.5f;
            float decelerationFactor = 1.0f - decelerationProgress;
            sf::Vector2f initialVelocity = aircraft.GetVelocityAtRelease();
            aircraft.SetVelocity(initialVelocity.x * decelerationFactor, initialVelocity.y * decelerationFactor);
        }
        else if (releaseTime >= 3.0f)
        {
            // Stop completely after 3 seconds
            aircraft.SetVelocity(0.f, 0.f);
        }
    }
};

Player::Player()
{
    m_key_binding[sf::Keyboard::Scancode::A] = Action::kMoveLeft;
    m_key_binding[sf::Keyboard::Scancode::D] = Action::kMoveRight;
    m_key_binding[sf::Keyboard::Scancode::W] = Action::kMoveUp;
    m_key_binding[sf::Keyboard::Scancode::S] = Action::kMoveDown;
    m_key_binding[sf::Keyboard::Scancode::Space] = Action::kBulletFire;
    m_key_binding[sf::Keyboard::Scancode::M] = Action::kMissileFire;

    InitialiseActions();

    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft);
    }

    m_was_forward_pressed = false;
}

void Player::HandleEvent(const sf::Event& event, CommandQueue& command_queue)
{
    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        auto found = m_key_binding.find(key_pressed->scancode);
        if (found != m_key_binding.end() && !IsRealTimeAction(found->second))
        {
            command_queue.Push(m_action_binding[found->second]);
        }
    }
}

void Player::HandleRealTimeInput(CommandQueue& command_queue)
{
    bool is_forward_currently_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::W);

    for (auto pair : m_key_binding)
    {
        if (sf::Keyboard::isKeyPressed(pair.first) && IsRealTimeAction(pair.second))
        {
            command_queue.Push(m_action_binding[pair.second]);
        }
    }

    // Reset forward acceleration time if W is not pressed but was pressed before
    if (!is_forward_currently_pressed && m_was_forward_pressed)
    {
        Command reset_command;
        reset_command.category = static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft);
        reset_command.action = DerivedAction<Aircraft>(AircraftForwardAccelerationReset());
        command_queue.Push(reset_command);
    }

    // Apply deceleration when W is not pressed
    if (!is_forward_currently_pressed)
    {
        Command decelerate_command;
        decelerate_command.category = static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft);
        decelerate_command.action = DerivedAction<Aircraft>(AircraftDecelerator());
        command_queue.Push(decelerate_command);
    }

    m_was_forward_pressed = is_forward_currently_pressed;
}

void Player::AssignKey(Action action, sf::Keyboard::Scancode key)
{
    //Remove keys that are currently bound to the action
    for (auto itr = m_key_binding.begin(); itr != m_key_binding.end();)
    {
        if (itr->second == action)
        {
            m_key_binding.erase(itr++);
        }
        else
        {
            ++itr;
        }
    }
    m_key_binding[key] = action;
}

sf::Keyboard::Scancode Player::GetAssignedKey(Action action) const
{
    for (auto pair : m_key_binding)
    {
        if (pair.second == action)
        {
            return pair.first;
        }
    }
    return sf::Keyboard::Scancode::Unknown;
}

void Player::SetMissionStatus(MissionStatus status)
{
    m_current_mission_status = status;
}

MissionStatus Player::GetMissionStatus() const
{
    return m_current_mission_status;
}

void Player::InitialiseActions()
{
    m_action_binding[Action::kMoveLeft].action = DerivedAction<Aircraft>(AircraftRotator(-kRotationSpeed));
    m_action_binding[Action::kMoveRight].action = DerivedAction<Aircraft>(AircraftRotator(kRotationSpeed));
    m_action_binding[Action::kMoveUp].action = DerivedAction<Aircraft>(AircraftForwardMover());
    m_action_binding[Action::kBulletFire].action = DerivedAction<Aircraft>([](Aircraft& a, sf::Time dt)
        {
            a.Fire();
        }
    );
    m_action_binding[Action::kMissileFire].action = DerivedAction<Aircraft>([](Aircraft& a, sf::Time dt)
        {
            a.LaunchMissile();
        }
    );

}

bool Player::IsRealTimeAction(Action action)
{
    switch (action)
    {
    case Action::kMoveLeft:
    case Action::kMoveRight:
    case Action::kMoveUp:
    case Action::kMoveDown:
    case Action::kBulletFire:
        return true;
    default:
        return false;
    }
}
