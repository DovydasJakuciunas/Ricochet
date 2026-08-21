#include "world.hpp"
#include "sprite_node.hpp"
#include <iostream>
#include "state.hpp"
#include <SFML/System/Angle.hpp>

World::World(sf::RenderWindow& window, FontHolder* font)
	: m_window(window)
	, m_camera(window.getDefaultView())
	, m_textures()
	, m_fonts(font)
	, m_scene_graph()
	, m_scene_layers()
	, m_world_bounds(sf::Vector2f(0.f, 0.f), sf::Vector2f(m_camera.getSize().x, 1000.f))
	, m_spawn_position(m_camera.getSize().x / 2.f, m_world_bounds.size.y - m_camera.getSize().y / 2.f)
	, m_scroll_speed(-100.f)
	, m_player_aircraft(nullptr)
{
	LoadTextures();
	BuildScene();
	m_camera.setCenter(m_spawn_position);
}

void World::Update(const sf::Time& dt)
{

	/* 
	//Scroll the world
	m_camera.move(sf::Vector2f(0, m_scroll_speed * dt.asSeconds()));
	*/

	m_player_aircraft->SetVelocity(0.f, 0.f);

	while (!m_command_queue.IsEmpty())
	{
		m_scene_graph.OnCommand(m_command_queue.Pop(), dt);
	}

	AdaptPlayerVelocity();

	m_scene_graph.Update(dt);
	AdaptPlayerPosition();
}

void World::Draw()
{
	m_window.setView(m_camera);
	m_window.draw(m_scene_graph);
}

CommandQueue& World::GetCommandQueue()
{
	return m_command_queue;
}

void World::LoadTextures()
{
	m_textures.Load(TextureID::kAlphaPlayer, "Media/Textures/AlphaPlayer.png");
	m_textures.Load(TextureID::kBetaPlayer, "Media/Textures/BetaPlayer.png");
	m_textures.Load(TextureID::kLandscape, "Media/Textures/Background.png");
}

void World::BuildScene()
{
	//Initialise the different layers
	for (int i = 0; i < static_cast<int>(SceneLayers::kLayerCount); i++)
	{
		SceneNode::Ptr layer(new SceneNode());
		m_scene_layers[i] = layer.get();
		m_scene_graph.AttachChild(std::move(layer));
	}

	//Prepare the background
	sf::Texture& texture = m_textures.Get(TextureID::kLandscape);
	sf::IntRect textureRect(m_world_bounds);
	texture.setRepeated(true);

	//Add the background sprite to the world
	std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, textureRect));
	background_sprite->setPosition(sf::Vector2f(0,0));
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));

	//Create the player aircraft
	std::unique_ptr<Aircraft> player(new Aircraft(AircraftType::kAlphaPlayer, m_textures, *m_fonts));
	m_player_aircraft = player.get();
	player->setPosition(m_spawn_position);
	m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(std::move(player));

}

void World::AdaptPlayerVelocity()
{
	
}

void World::AdaptPlayerPosition()
{
	//keep player on the screen
	sf::FloatRect view_bounds(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());
	const float border_distance = 40.f;

	sf::Vector2f position = m_player_aircraft->getPosition();
	position.x = std::min(position.x, view_bounds.size.x - border_distance);
	position.x = std::max(position.x, border_distance);
	position.y = std::min(position.y, view_bounds.position.y + view_bounds.size.y - border_distance);
	position.y = std::max(position.y, view_bounds.position.y + border_distance);

	m_player_aircraft->setPosition(position);

}