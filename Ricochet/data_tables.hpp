#pragma once
#include "texture_id.hpp"
#include <SFML/System/Time.hpp>

struct Direction
{
	Direction(float angle, float distance) : m_angle(angle), m_distance(distance) {}
		
	float m_angle;
	float m_distance;
};

struct AircraftData
{
	int m_hitpoints;
	float m_speed;
	TextureID m_texture;
	sf::Time m_fire_interval;
	std::vector<Direction> m_directions;
};

std::vector<AircraftData> InitializeAircraftData();

