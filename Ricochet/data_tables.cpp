#include "data_tables.hpp"
#include "aircraft_type.hpp"

std::vector<AircraftData> InitializeAircraftData()
{
	std::vector<AircraftData> data(static_cast<int>(AircraftType::kAircraftCount));

	data[static_cast<int>(AircraftType::kAlphaPlayer)].m_hitpoints = 100;
	data[static_cast<int>(AircraftType::kAlphaPlayer)].m_fire_interval = sf::seconds(1);
	data[static_cast<int>(AircraftType::kAlphaPlayer)].m_texture = TextureID::kAlphaPlayer;

	return data;
}
