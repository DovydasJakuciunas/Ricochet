#pragma once
#include <SFML/Graphics.hpp>
#include "scene_node.hpp"
#include "resource_holder.hpp"
#include "texture_id.hpp"


class Game
{
public:
	Game();
	void Run();

private:
	void ProcessEvents();
	void Update(sf::Time delta_time);
	void Render();
	void HandlePlayerInput(sf::Keyboard::Scancode key, bool is_pressed);

private:
	sf::RenderWindow m_window;
	ResourceHolder<TextureID, sf::Texture> m_textures;
	std::unique_ptr<sf::Sprite> m_player;

	bool m_is_accelerating = false;  
	bool m_is_decelerating = false;  
	bool m_is_rotating_left = false;  
	bool m_is_rotating_right = false; 

	float m_current_speed = 0.f;     
	float m_rotation = 0.f;          
};

