#include "aircraft.hpp"
#include "texture_id.hpp"
#include "constants.hpp"
#include <cmath>
#include "data_tables.hpp"
#include "utility.hpp"

namespace
{
	const std::vector<AircraftData> Table = InitializeAircraftData();
}

TextureID ToTextureID(AircraftType type)
{
	switch (type)
	{
	case AircraftType::kAlphaPlayer:
		return TextureID::kAlphaPlayer;
		break;
	case AircraftType::kBetaPlayer:
		return TextureID::kBetaPlayer;
		break;
	}
	return TextureID::kAlphaPlayer;
}

Aircraft::Aircraft(AircraftType type, const TextureHolder& textures, const FontHolder& fonts) : Entity(Table[static_cast<int>(type)]).m_hitpoints), m_type(type), m_sprite(textures.Get(ToTextureID(type))), m_health_display(nullptr), m_distance_travelled(0.f), m_directions_index(0)
{
	sf::FloatRect bounds = m_sprite.getLocalBounds();
	m_sprite.setOrigin(bounds.getCenter());
	std::string* health = new std::string("");
	std::unique_ptr<TextNode> health_display(new TextNode(fonts, *health));
	m_health_display = health_display.get();
	AttachChild(std::move(health_display));
	UpdateTexts();
}

unsigned int Aircraft::GetCategory() const
{
	switch (m_type)
	{
	case AircraftType::kAlphaPlayer:
		return static_cast<unsigned int>(ReceiverCategories::kLocalPlayer);
	case AircraftType::kBetaPlayer:
		return static_cast<unsigned int>(ReceiverCategories::kForeignPlayer);
	}
	return 0;
}

void Aircraft::UpdateTexts()
{
	m_health_display->SetString(std::to_string(GetHitPoints()) + "HP");
	m_health_display->setPosition(sf::Vector2f(0.f, 50.f));
	m_health_display->setRotation(-getRotation());
}

void Aircraft::SetAccelerating(bool accelerating)
{
	m_is_accelerating = accelerating;
}

void Aircraft::SetDecelerating(bool decelerating)
{
	m_is_decelerating = decelerating;
}

void Aircraft::SetRotatingLeft(bool rotating)
{
	m_is_rotating_left = rotating;
}

void Aircraft::SetRotatingRight(bool rotating)
{
	m_is_rotating_right = rotating;
}

void Aircraft::UpdateCurrent(const sf::Time& dt)
{
	const float delta_time = dt.asSeconds();

	// Handle Speed of Player
	if (m_is_accelerating)
	{
		m_current_speed = std::min(m_current_speed + kPlayerAcceleration * delta_time, kPlayerSpeed);
	}
	else if (m_is_decelerating)
	{
		m_current_speed = std::max(m_current_speed - kPlayerDeceleration * delta_time, 0.f);
	}
	else
	{
		// Physics based drag: player starts losing speed if not accelerating
		float drag_deceleration = (drag_coefficient * m_current_speed * m_current_speed) / kPlayerSpeed;
		m_current_speed = std::max(m_current_speed - drag_deceleration * delta_time, 0.f);
	}

	// Handle Rotation Speed
	if (m_is_rotating_left)
	{
		m_rotation -= kPlayerRotationSpeed * delta_time;
	}
	if (m_is_rotating_right)
	{
		m_rotation += kPlayerRotationSpeed * delta_time;
	}

	setRotation(sf::degrees(m_rotation + 180));

	// Starting at 90 for SFML, Calculating Players angle, converts to unit vector and moves the player
	const float angle_rad = (m_rotation + 90.f) * kPieValue / 180.f;
	sf::Vector2f direction(std::cos(angle_rad), std::sin(angle_rad));
	move(direction * m_current_speed * delta_time);

	// Clear flags at the end of update - they're only true while input is active this frame
	m_is_accelerating = false;
	m_is_decelerating = false;
	m_is_rotating_left = false;
	m_is_rotating_right = false;
}

void Aircraft::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_sprite, states);
}
