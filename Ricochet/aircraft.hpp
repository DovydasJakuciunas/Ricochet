#pragma once
#include "entity.hpp"
#include "aircraft_type.hpp"
#include "resource_identifiers.hpp"
#include "receiver_categories.hpp"
#include "text_node.hpp"

class Aircraft : public Entity 
{
public:
	Aircraft(AircraftType type, const TextureHolder& textures, const FontHolder& fonts);
	unsigned int GetCategory() const override;

	void SetAccelerating(bool accelerating);
	void SetDecelerating(bool decelerating);
	void SetRotatingLeft(bool rotating);
	void SetRotatingRight(bool rotating);

	void UpdateTexts();

private:
	virtual void UpdateCurrent(const sf::Time& dt) override;
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;

private:
	AircraftType m_type;
	sf::Sprite m_sprite;
	float m_current_speed = 0.f;
	float m_rotation = 0.f;
	bool m_is_accelerating = false;
	bool m_is_decelerating = false;
	bool m_is_rotating_left = false;
	bool m_is_rotating_right = false;

	TextNode* m_health_display;
	float m_distance_travelled = 0.f;
	int m_directions_index;

};

