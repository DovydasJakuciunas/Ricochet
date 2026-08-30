#include "world.hpp"
#include "sprite_node.hpp"
#include <iostream>
#include <random>
#include "state.hpp"
#include <SFML/System/Angle.hpp>
#include "Projectile.hpp"
#include "pickup.hpp"
#include "particle_node.hpp"
#include "particletype.hpp"
#include "sound_node.hpp"
#include "entity.hpp"

World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds)
	: m_target(output_target)
	, m_camera(output_target.getDefaultView())
	, m_textures()
	, m_fonts(font)
	, m_sounds(sounds)
	, m_scene_graph(ReceiverCategories::kNone)
	, m_scene_layers()
	, m_world_bounds(sf::Vector2f(0.f, 0.f), sf::Vector2f(m_camera.getSize().x, m_camera.getSize().y))
	, m_spawn_position(Utility::RandomFloat(0.f, m_camera.getSize().x), Utility::RandomFloat(0.f, m_camera.getSize().y))
	, m_player_aircraft(nullptr)
{
	m_scene_texture.resize({ m_target.getSize().x, m_target.getSize().y });
	LoadTextures();

	BuildScene();
}

void World::Update(sf::Time dt)
{

	GuideMissiles();

	UpdateSounds();

	//Process commands from the scenegraph
	while (!m_command_queue.IsEmpty())
	{
		m_scene_graph.OnCommand(m_command_queue.Pop(), dt);
	}
	HandleCollisions();
	m_scene_graph.RemoveWrecks();

	m_scene_graph.Update(dt, m_command_queue);

	BounceProjectiles();
	AdaptPlayerPosition();
}

void World::Draw()
{
	if (PostEffect::IsSupported())
	{
		m_scene_texture.clear();
		m_scene_texture.setView(m_camera);
		m_scene_texture.draw(m_scene_graph);
		m_scene_texture.display();
		m_bloom_effect.Apply(m_scene_texture, m_target);
	}
	else
	{
		m_target.setView(m_camera);
		m_target.draw(m_scene_graph);
	}
}

CommandQueue& World::GetCommandQueue()
{
	return m_command_queue;
}

bool World::HasAlivePlayer() const
{
	return !m_player_aircraft->IsMarkedForRemoval();
}

void World::LoadTextures()
{
	m_textures.Load(TextureID::kEntities, "Media/Textures/Entities.png");
	m_textures.Load(TextureID::kExplosion, "Media/Textures/Explosion.png");
	
	m_textures.Load(TextureID::kBackground, "Media/Textures/Background.png");
	m_textures.Load(TextureID::kParticle, "Media/Textures/Particle.png");
}

void World::BuildScene()
{
	//Initialise the different layers
	for (int i = 0; i < static_cast<int>(SceneLayers::kLayerCount); i++)
	{
		ReceiverCategories category = (i == static_cast<int>(SceneLayers::kLowerAir)) ? ReceiverCategories::kScene : ReceiverCategories::kNone;
		SceneNode::Ptr layer(new SceneNode(category));
		m_scene_layers[i] = layer.get();
		m_scene_graph.AttachChild(std::move(layer));
	}

	//Prepare the background
	sf::Texture& texture = m_textures.Get(TextureID::kBackground);
	sf::IntRect textureRect(m_world_bounds);
	texture.setRepeated(true);

	//Add the background sprite to the world
	std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, textureRect));
	background_sprite->setPosition(sf::Vector2f(0.f, 0.f));
	m_background_sprite = background_sprite.get();
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));

	std::unique_ptr<Aircraft> leader(new Aircraft(AircraftType::kEagle, m_textures, m_fonts));
	m_player_aircraft = leader.get();
	m_player_aircraft->setPosition(m_spawn_position);
	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(leader));

	//Add the particle nodes to the scene
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleType::kSmoke, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(smokeNode));

	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleType::kPropellant, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(propellantNode));

	//Add sound effect node
	std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
	m_scene_graph.AttachChild(std::move(soundNode));
}

void World::AdaptPlayerPosition()
{
	HandlePlayerBoundaryCollision();
}

void World::HandlePlayerBoundaryCollision()
{
	// Keep player within the camera view bounds with a border
	sf::FloatRect view_bounds(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());
	const float border_distance = 40.f;

	sf::Vector2f position = m_player_aircraft->getPosition();
	sf::FloatRect player_bounds = m_player_aircraft->GetBoundingRect();

	// Keep player within bounds (invert velocity and rotation on wall collision)
	if (player_bounds.position.x <= view_bounds.position.x + border_distance)
	{
		// Hit left boundary - invert X velocity and rotation
		position.x = view_bounds.position.x + border_distance + (player_bounds.size.x / 2.f);
		m_player_aircraft->InvertVelocityX();
		m_player_aircraft->InvertRotation();
	}
	else if (player_bounds.position.x + player_bounds.size.x >= view_bounds.position.x + view_bounds.size.x - border_distance)
	{
		// Hit right boundary - invert X velocity and rotation
		position.x = view_bounds.position.x + view_bounds.size.x - border_distance - (player_bounds.size.x / 2.f);
		m_player_aircraft->InvertVelocityX();
		m_player_aircraft->InvertRotation();
	}

	// Keep player within bounds vertically
	if (player_bounds.position.y <= view_bounds.position.y + border_distance)
	{
		// Hit top boundary - invert Y velocity and rotation
		position.y = view_bounds.position.y + border_distance + (player_bounds.size.y / 2.f);
		m_player_aircraft->InvertVelocityY();
		m_player_aircraft->InvertRotation();
	}
	else if (player_bounds.position.y + player_bounds.size.y >= view_bounds.position.y + view_bounds.size.y - border_distance)
	{
		// Hit bottom boundary - invert Y velocity and rotation
		position.y = view_bounds.position.y + view_bounds.size.y - border_distance - (player_bounds.size.y / 2.f);
		m_player_aircraft->InvertVelocityY();
		m_player_aircraft->InvertRotation();
	}

	m_player_aircraft->setPosition(position);
}

sf::FloatRect World::GetViewBounds() const
{
	return sf::FloatRect(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());;
}

sf::FloatRect World::GetBattleFieldBounds() const
{
	//Return camera bounds + a small area off screen where the enemies spawn
	sf::FloatRect bounds = GetViewBounds();
	bounds.position.y -= 100.f;
	bounds.size.y += 100.f;
	return bounds;
}

void World::GuideMissiles()
{
	//TODO - Change this so it targets the other player
	////Target the closest enemy in the world
	//Command enemyCollector;
	//enemyCollector.category = static_cast<int>(ReceiverCategories::kEnemyAircraft);
	//enemyCollector.action = DerivedAction<Aircraft>([this](Aircraft& enemy, sf::Time)
	//	{
	//		if (!enemy.IsDestroyed())
	//		{
	//			m_active_enemies.emplace_back(&enemy);
	//		}
	//	});

	//Command missileGuider;
	//missileGuider.category = static_cast<int>(ReceiverCategories::kAlliedProjectile);
	//missileGuider.action = DerivedAction<Projectile>([this](Projectile& missile, sf::Time)
	//	{
	//		if (!missile.IsGuided())
	//		{
	//			return;
	//		}

	//		float min_distance = std::numeric_limits<float>::max();
	//		Aircraft* closest_enemy = nullptr;

	//		for (Aircraft* enemy : m_active_enemies)
	//		{
	//			float enemy_distance = Distance(missile, *enemy);
	//			if (enemy_distance < min_distance)
	//			{
	//				closest_enemy = enemy;
	//				min_distance = enemy_distance;
	//			}
	//		}
	//		if (closest_enemy)
	//		{
	//			missile.GuideTowards(closest_enemy->GetWorldPosition());
	//		}
	//	});
	//m_command_queue.Push(enemyCollector);
	//m_command_queue.Push(missileGuider);
	//m_active_enemies.clear();
}

bool MatchesCategories(SceneNode::Pair& colliders, ReceiverCategories type1, ReceiverCategories type2)
{
	unsigned int category1 = colliders.first->GetCategory();
	unsigned int category2 = colliders.second->GetCategory();

	if ((static_cast<int>(type1) & category1) && (static_cast<int>(type2) & category2))
	{
		return true;
	}
	else if ((static_cast<int>(type1) & category2) && (static_cast<int>(type2) & category1))
	{
		std::swap(colliders.first, colliders.second);
		return true;
	}
	else
	{
		return false;
	}

}

void World::HandleCollisions()
{
	std::set<SceneNode::Pair> collision_pairs;
	m_scene_graph.CheckSceneCollision(m_scene_graph, collision_pairs);

	for (SceneNode::Pair pair : collision_pairs)
	{
		if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kEnemyAircraft))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& enemy = static_cast<Aircraft&>(*pair.second);
			//Collision response
			player.Damage(enemy.GetHitPoints());
			enemy.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kPickup))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);
			//Collision response
			pickup.Apply(player);
			pickup.Destroy();
			player.PlayLocalSound(m_command_queue, SoundEffect::kCollectPickup);
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kEnemyProjectile) || MatchesCategories(pair, ReceiverCategories::kEnemyAircraft, ReceiverCategories::kAlliedProjectile))
		{
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);
			//Collision response
			aircraft.Damage(projectile.GetDamage());
			projectile.Destroy();
		}
	}
}

void World::UpdateSounds()
{
	sf::Vector2f listener_position;

	listener_position = m_camera.getCenter();

	m_sounds.SetListenerPosition(listener_position);

	m_sounds.RemoveStoppedSounds();
}

void World::BounceProjectiles()
{
	// Apply bouncing to all projectiles in the scene
	Command projectileBouncer;
	projectileBouncer.category = static_cast<int>(ReceiverCategories::kProjectile);
	projectileBouncer.action = DerivedAction<SceneNode>([this](SceneNode& node, sf::Time)
		{
			BounceEntity(&node);
		});
	m_command_queue.Push(projectileBouncer);
}

void World::BounceEntity(SceneNode* entity)
{
	if (!entity)
		return;

	// Get entity bounds and velocity
	sf::FloatRect bounds = entity->GetBoundingRect();

	// Cast to Entity to access velocity methods
	Entity* moving_entity = dynamic_cast<Entity*>(entity);
	if (!moving_entity)
		return;

	sf::Vector2f velocity = moving_entity->GetVelocity();
	sf::Vector2f position = entity->getPosition();

	// Check collision with left and right walls
	if (bounds.position.x <= m_world_bounds.position.x)
	{
		// Hit left wall - bounce back right
		velocity.x = std::abs(velocity.x);
		position.x = m_world_bounds.position.x + (bounds.size.x / 2.f);
	}
	else if (bounds.position.x + bounds.size.x >= m_world_bounds.position.x + m_world_bounds.size.x)
	{
		// Hit right wall - bounce back left
		velocity.x = -std::abs(velocity.x);
		position.x = m_world_bounds.position.x + m_world_bounds.size.x - (bounds.size.x / 2.f);
	}

	// Check collision with top and bottom walls
	if (bounds.position.y <= m_world_bounds.position.y)
	{
		// Hit top wall - bounce down
		velocity.y = std::abs(velocity.y);
		position.y = m_world_bounds.position.y + (bounds.size.y / 2.f);
	}
	else if (bounds.position.y + bounds.size.y >= m_world_bounds.position.y + m_world_bounds.size.y)
	{
		// Hit bottom wall - bounce up
		velocity.y = -std::abs(velocity.y);
		position.y = m_world_bounds.position.y + m_world_bounds.size.y - (bounds.size.y / 2.f);
	}

	// Apply new velocity and position
	moving_entity->SetVelocity(velocity);
	entity->setPosition(position);
}


