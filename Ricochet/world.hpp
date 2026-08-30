#pragma once
#include <SFML/Graphics.hpp>
#include "resource_identifiers.hpp"
#include "scene_node.hpp"
#include "scene_layers.hpp"
#include "aircraft.hpp"
#include "command_queue.hpp"
#include "bloom_effect.hpp"
#include "sound_player.hpp"
#include "sprite_node.hpp"
#include "utility.hpp"

class World
{
public:
	explicit World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds);
	void Update(sf::Time dt);
	void Draw();

	CommandQueue& GetCommandQueue();

	bool HasAlivePlayer() const;

private:
	void LoadTextures();
	void BuildScene();
	void AdaptPlayerPosition();
	void HandlePlayerBoundaryCollision();

	sf::FloatRect GetViewBounds() const;
	sf::FloatRect GetBattleFieldBounds() const;

	void GuideMissiles();

	void HandleCollisions();

	void UpdateSounds();

	void BounceProjectiles();
	void BounceEntity(SceneNode* entity);
	void SpawnRandomPickups();

private:
	sf::RenderTarget& m_target;
	sf::RenderTexture m_scene_texture;
	sf::View m_camera;
	TextureHolder m_textures;
	FontHolder& m_fonts;
	SoundPlayer& m_sounds;
	SceneNode m_scene_graph;
	std::array<SceneNode*, static_cast<int>(SceneLayers::kLayerCount)> m_scene_layers;
	sf::FloatRect m_world_bounds;
	sf::Vector2f m_spawn_position;
	Aircraft* m_player_aircraft;

	CommandQueue m_command_queue;
	Command m_command;

	BloomEffect m_bloom_effect;
	SpriteNode* m_background_sprite;
	sf::Time m_pickup_spawn_timer;
};

