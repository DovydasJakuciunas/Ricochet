#include "aircraft.hpp"
#include "texture_id.hpp"

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

Aircraft::Aircraft(AircraftType type, const TextureHolder& textures) : m_type(type), m_sprite(textures.Get(ToTextureID(type)))
{
	sf::FloatRect bounds = m_sprite.getLocalBounds();
	m_sprite.setOrigin(bounds.getCenter());
}

void Aircraft::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_sprite, states);
}