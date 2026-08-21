#include "player.hpp"
#include "aircraft.hpp"
#include "constants.hpp"
#include <cmath>

struct AircraftRotator
{
	AircraftRotator(bool rotating_left) : m_rotating_left(rotating_left) {}
	void operator()(Aircraft& aircraft, sf::Time) const
	{
		if (m_rotating_left)
		{
			aircraft.SetRotatingLeft(true);
		}
		else
		{
			aircraft.SetRotatingRight(true);
		}
	}

	bool m_rotating_left;
};

struct AircraftAccelerator
{
	AircraftAccelerator(bool accelerating) : m_accelerating(accelerating) {}
	void operator()(Aircraft& aircraft, sf::Time) const
	{
		if (m_accelerating)
		{
			aircraft.SetAccelerating(true);
		}
		else
		{
			aircraft.SetDecelerating(true);
		}
	}

	bool m_accelerating;
};

Player::Player()
{
	m_key_binding[sf::Keyboard::Scancode::A] = Action::kRotateLeft;
	m_key_binding[sf::Keyboard::Scancode::D] = Action::kRotateRight;
	m_key_binding[sf::Keyboard::Scancode::W] = Action::kAccelerate;
	m_key_binding[sf::Keyboard::Scancode::S] = Action::kDecelerate;

	InitialiseActions();

	for (auto& pair : m_action_binding)
	{
		pair.second.category = static_cast<unsigned int>(ReceiverCategories::kLocalPlayer);
	}
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
	for (auto pair : m_key_binding)
	{
		if(sf::Keyboard::isKeyPressed(pair.first) && IsRealTimeAction(pair.second))
		{
			command_queue.Push(m_action_binding[pair.second]);
		}
	}
}

void Player::AssignKey(Action action, sf::Keyboard::Scancode key)
{
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

void Player::InitialiseActions()
{
	m_action_binding[Action::kRotateLeft].action = DerivedAction<Aircraft>(AircraftRotator(true));
	m_action_binding[Action::kRotateRight].action = DerivedAction<Aircraft>(AircraftRotator(false));
	m_action_binding[Action::kAccelerate].action = DerivedAction<Aircraft>(AircraftAccelerator(true));
	m_action_binding[Action::kDecelerate].action = DerivedAction<Aircraft>(AircraftAccelerator(false));
}

bool Player::IsRealTimeAction(Action action)
{
	switch (action)
	{
	case Action::kRotateLeft:
	case Action::kRotateRight:
	case Action::kAccelerate:
	case Action::kDecelerate:
		return true;
	default:
		return false;
	}
	return false;
}