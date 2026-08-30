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
	, m_spawn_position_p2(sf::Vector2f(0.f, 0.f))
	, m_player_aircraft(nullptr)
	, m_player_aircraft_p2(nullptr)
	, m_player1_kills(0)
	, m_player2_kills(0)
	, m_player1_was_alive(true)
	, m_player2_was_alive(true)
	, m_player1_kill_display(nullptr)
	, m_player2_kill_display(nullptr)
	, m_pickup_spawn_timer(sf::seconds(0.f))
{
	m_scene_texture.resize({ m_target.getSize().x, m_target.getSize().y });
	LoadTextures();

	BuildScene();
}

void World::Update(sf::Time dt)
{

	GuideMissiles();

	//UpdateSounds();

	//Process commands from the scenegraph
	while (!m_command_queue.IsEmpty())
	{
		m_scene_graph.OnCommand(m_command_queue.Pop(), dt);
	}
	HandleCollisions();

	// Detect kills BEFORE RemoveWrecks - check if players changed from alive to dead
	bool player1_alive = m_player_aircraft && !m_player_aircraft->IsMarkedForRemoval();
	bool player2_alive = m_player_aircraft_p2 && !m_player_aircraft_p2->IsMarkedForRemoval();

	// Player 1 was alive but is now dead - Player 2 gets a kill and Player 1 respawns
	if (m_player1_was_alive && !player1_alive && m_player_aircraft)
	{
		IncrementPlayer2Kills();
		// Respawn Player 1 BEFORE RemoveWrecks
		m_player_aircraft->Respawn();
		m_player_aircraft->setPosition(m_spawn_position);
		player1_alive = true;
	}

	// Player 2 was alive but is now dead - Player 1 gets a kill and Player 2 respawns
	if (m_player2_was_alive && !player2_alive && m_player_aircraft_p2)
	{
		IncrementPlayer1Kills();
		// Respawn Player 2 BEFORE RemoveWrecks
		m_player_aircraft_p2->Respawn();
		m_player_aircraft_p2->setPosition(m_spawn_position_p2);
		player2_alive = true;
	}

	// Update alive status for next frame
	m_player1_was_alive = player1_alive;
	m_player2_was_alive = player2_alive;

	m_scene_graph.RemoveWrecks();

	// Update kill count displays
	if (m_player1_kill_display)
	{
		m_player1_kill_display->SetString("P1 Kills: " + std::to_string(m_player1_kills));
	}
	if (m_player2_kill_display)
	{
		m_player2_kill_display->SetString("P2 Kills: " + std::to_string(m_player2_kills));
	}

	m_scene_graph.Update(dt, m_command_queue);

	SpawnRandomPickups();

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

	std::unique_ptr<Aircraft> leader(new Aircraft(AircraftType::kEagle, m_textures, m_fonts, PlayerID::kPlayer1));
	m_player_aircraft = leader.get();
	m_player_aircraft->setPosition(m_spawn_position);
	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(leader));

	// Spawn Player 2 at opposite side of the map
	// Mirror the spawn position: if P1 is at (x, y), P2 spawns at (width - x, height - y)
	m_spawn_position_p2 = sf::Vector2f(
		m_world_bounds.position.x + m_world_bounds.size.x - m_spawn_position.x,
		m_world_bounds.position.y + m_world_bounds.size.y - m_spawn_position.y
	);

	std::unique_ptr<Aircraft> player2(new Aircraft(AircraftType::kEagle, m_textures, m_fonts, PlayerID::kPlayer2));
	m_player_aircraft_p2 = player2.get();
	m_player_aircraft_p2->setPosition(m_spawn_position_p2);
	m_player_aircraft_p2->rotate(sf::degrees(180.f));  // Face opposite direction
	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(player2));

	//Add the particle nodes to the scene
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleType::kSmoke, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(smokeNode));

	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleType::kPropellant, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(propellantNode));

	// Create kill count displays
	std::string* p1_kill_text = new std::string("Kills: 0");
	std::unique_ptr<TextNode> p1_kill_display(new TextNode(m_fonts, *p1_kill_text));
	m_player1_kill_display = p1_kill_display.get();
	m_player1_kill_display->setPosition(sf::Vector2f(20.f, 20.f));
	m_player1_kill_display->setScale(sf::Vector2f(0.8f, 0.8f));
	m_scene_graph.AttachChild(std::move(p1_kill_display));

	std::string* p2_kill_text = new std::string("Kills: 0");
	std::unique_ptr<TextNode> p2_kill_display(new TextNode(m_fonts, *p2_kill_text));
	m_player2_kill_display = p2_kill_display.get();
	m_player2_kill_display->setPosition(sf::Vector2f(m_camera.getSize().x - 200.f, 20.f));
	m_player2_kill_display->setScale(sf::Vector2f(0.8f, 0.8f));
	m_scene_graph.AttachChild(std::move(p2_kill_display));

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

	// Handle Player 1 collision
	if (m_player_aircraft)
	{
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

	// Handle Player 2 collision (same logic)
	if (m_player_aircraft_p2)
	{
		sf::Vector2f position = m_player_aircraft_p2->getPosition();
		sf::FloatRect player_bounds = m_player_aircraft_p2->GetBoundingRect();

		// Keep player within bounds (invert velocity and rotation on wall collision)
		if (player_bounds.position.x <= view_bounds.position.x + border_distance)
		{
			// Hit left boundary - invert X velocity and rotation
			position.x = view_bounds.position.x + border_distance + (player_bounds.size.x / 2.f);
			m_player_aircraft_p2->InvertVelocityX();
			m_player_aircraft_p2->InvertRotation();
		}
		else if (player_bounds.position.x + player_bounds.size.x >= view_bounds.position.x + view_bounds.size.x - border_distance)
		{
			// Hit right boundary - invert X velocity and rotation
			position.x = view_bounds.position.x + view_bounds.size.x - border_distance - (player_bounds.size.x / 2.f);
			m_player_aircraft_p2->InvertVelocityX();
			m_player_aircraft_p2->InvertRotation();
		}

		// Keep player within bounds vertically
		if (player_bounds.position.y <= view_bounds.position.y + border_distance)
		{
			// Hit top boundary - invert Y velocity and rotation
			position.y = view_bounds.position.y + border_distance + (player_bounds.size.y / 2.f);
			m_player_aircraft_p2->InvertVelocityY();
			m_player_aircraft_p2->InvertRotation();
		}
		else if (player_bounds.position.y + player_bounds.size.y >= view_bounds.position.y + view_bounds.size.y - border_distance)
		{
			// Hit bottom boundary - invert Y velocity and rotation
			position.y = view_bounds.position.y + view_bounds.size.y - border_distance - (player_bounds.size.y / 2.f);
			m_player_aircraft_p2->InvertVelocityY();
			m_player_aircraft_p2->InvertRotation();
		}

		m_player_aircraft_p2->setPosition(position);
	}
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
		// Player-to-Player collision
		if ((MatchesCategories(pair, ReceiverCategories::kPlayer1Aircraft, ReceiverCategories::kPlayer2Aircraft)))
		{
			auto& player1 = static_cast<Aircraft&>(*pair.first);
			auto& player2 = static_cast<Aircraft&>(*pair.second);

			// Skip collision if either player is immune
			if (player1.IsCollisionImmune() || player2.IsCollisionImmune())
			{
				continue;
			}

			// Calculate velocity magnitudes
			sf::Vector2f vel1 = player1.GetVelocity();
			sf::Vector2f vel2 = player2.GetVelocity();
			float speed1 = std::sqrt(vel1.x * vel1.x + vel1.y * vel1.y);
			float speed2 = std::sqrt(vel2.x * vel2.x + vel2.y * vel2.y);

			// Calculate bounce direction (from player1 to player2)
			sf::Vector2f dir = player2.GetWorldPosition() - player1.GetWorldPosition();
			float distance = std::sqrt(dir.x * dir.x + dir.y * dir.y);
			if (distance > 0.f)
			{
				dir /= distance;  // Normalize
			}
			else
			{
				dir = sf::Vector2f(1.f, 0.f);  // Default direction if at same position
			}

			// Bounce velocity magnitude
			constexpr float kBounceForce = 300.f;

			// Apply bounce velocities (push players apart)
			player1.SetVelocity(player1.GetVelocity() - dir * kBounceForce);
			player2.SetVelocity(player2.GetVelocity() + dir * kBounceForce);

			// Grace period duration (0.5 seconds)
			constexpr sf::Time kCollisionGracePeriod = sf::milliseconds(500);

			// Only the slower player takes damage
			if (speed1 < speed2)
			{
				player1.Damage(10);
				player1.SetCollisionImmunity(kCollisionGracePeriod);
			}
			else if (speed2 < speed1)
			{
				player2.Damage(10);
				player2.SetCollisionImmunity(kCollisionGracePeriod);
			}
			// If speeds are equal, both take damage and both get immunity
			else
			{
				player1.Damage(10);
				player2.Damage(10);
				player1.SetCollisionImmunity(kCollisionGracePeriod);
				player2.SetCollisionImmunity(kCollisionGracePeriod);
			}
		}
		// Legacy single-player collision handling
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kEnemyAircraft))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& enemy = static_cast<Aircraft&>(*pair.second);
			//Collision response
			player.Damage(enemy.GetHitPoints());
			enemy.Destroy();
		}
		// Pickup collection - Player 1
		else if (MatchesCategories(pair, ReceiverCategories::kPlayer1Aircraft, ReceiverCategories::kPickup))
		{
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);
			//Collision response
			pickup.Apply(aircraft);
			pickup.Destroy();
			aircraft.PlayLocalSound(m_command_queue, SoundEffect::kCollectPickup);
		}
		// Pickup collection - Player 2
		else if (MatchesCategories(pair, ReceiverCategories::kPlayer2Aircraft, ReceiverCategories::kPickup))
		{
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);
			//Collision response
			pickup.Apply(aircraft);
			pickup.Destroy();
			aircraft.PlayLocalSound(m_command_queue, SoundEffect::kCollectPickup);
		}
		// Player 1 hit by any projectile (check owner for PvP)
		else if (MatchesCategories(pair, ReceiverCategories::kPlayer1Aircraft, ReceiverCategories::kAlliedProjectile))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);
			// In PvP, Player 1 can be damaged by Player 2's projectiles
			if (projectile.GetOwnerPlayerID() == PlayerID::kPlayer2)
			{
				player.Damage(projectile.GetDamage());
				projectile.Destroy();
			}
		}
		// Player 1 hit by enemy projectile
		else if (MatchesCategories(pair, ReceiverCategories::kPlayer1Aircraft, ReceiverCategories::kEnemyProjectile))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);
			//Collision response
			player.Damage(projectile.GetDamage());
			projectile.Destroy();
		}
		// Player 2 hit by any projectile (check owner for PvP)
		else if (MatchesCategories(pair, ReceiverCategories::kPlayer2Aircraft, ReceiverCategories::kAlliedProjectile))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);
			// In PvP, Player 2 can be damaged by Player 1's projectiles
			if (projectile.GetOwnerPlayerID() == PlayerID::kPlayer1)
			{
				player.Damage(projectile.GetDamage());
				projectile.Destroy();
			}
		}
		// Player 2 hit by enemy projectile
		else if (MatchesCategories(pair, ReceiverCategories::kPlayer2Aircraft, ReceiverCategories::kEnemyProjectile))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);
			//Collision response
			player.Damage(projectile.GetDamage());
			projectile.Destroy();
		}
		// Legacy enemy hit by player projectile
		else if (MatchesCategories(pair, ReceiverCategories::kEnemyAircraft, ReceiverCategories::kAlliedProjectile))
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

	bool bounced = false;

	// Check collision with left and right walls
	if (bounds.position.x <= m_world_bounds.position.x)
	{
		// Hit left wall - bounce back right
		velocity.x = std::abs(velocity.x);
		position.x = m_world_bounds.position.x + (bounds.size.x / 2.f);
		bounced = true;
	}
	else if (bounds.position.x + bounds.size.x >= m_world_bounds.position.x + m_world_bounds.size.x)
	{
		// Hit right wall - bounce back left
		velocity.x = -std::abs(velocity.x);
		position.x = m_world_bounds.position.x + m_world_bounds.size.x - (bounds.size.x / 2.f);
		bounced = true;
	}

	// Check collision with top and bottom walls
	if (bounds.position.y <= m_world_bounds.position.y)
	{
		// Hit top wall - bounce down
		velocity.y = std::abs(velocity.y);
		position.y = m_world_bounds.position.y + (bounds.size.y / 2.f);
		bounced = true;
	}
	else if (bounds.position.y + bounds.size.y >= m_world_bounds.position.y + m_world_bounds.size.y)
	{
		// Hit bottom wall - bounce up
		velocity.y = -std::abs(velocity.y);
		position.y = m_world_bounds.position.y + m_world_bounds.size.y - (bounds.size.y / 2.f);
		bounced = true;
	}

	// If projectile bounced, increment bounce count and check limit
	if (bounced)
	{
		Projectile* projectile = dynamic_cast<Projectile*>(entity);
		if (projectile)
		{
			projectile->IncrementBounceCount();
			if (projectile->HasExceededBounceLimit())
			{
				projectile->Destroy();
				return;  // Don't apply velocity if destroying
			}
		}
	}

	// Apply new velocity and position
	moving_entity->SetVelocity(velocity);
	entity->setPosition(position);
}

void World::SpawnRandomPickups()
{
	// Spawn pickups every 3 seconds using a fixed timer
	m_pickup_spawn_timer -= sf::seconds(1.f / 60.f);  // Assuming 60 FPS

	if (m_pickup_spawn_timer <= sf::seconds(0.f))
	{
		m_pickup_spawn_timer = sf::seconds(3.f);  // Reset timer to 3 seconds

		sf::FloatRect view_bounds = GetViewBounds();

		// Add a border margin to keep pickups away from edges
		const float border = 50.f;
		sf::FloatRect spawn_area(
			sf::Vector2f(view_bounds.position.x + border, view_bounds.position.y + border),
			sf::Vector2f(view_bounds.size.x - (border * 2.f), view_bounds.size.y - (border * 2.f))
		);

		// Random pickup type
		PickupType type = static_cast<PickupType>(Utility::RandomInt(static_cast<int>(PickupType::kPickupCount)));

		// Spawn at random position within camera view (with border)
		float random_x = spawn_area.position.x + Utility::RandomInt(static_cast<int>(spawn_area.size.x));
		float random_y = spawn_area.position.y + Utility::RandomInt(static_cast<int>(spawn_area.size.y));

		std::unique_ptr<Pickup> pickup(new Pickup(type, m_textures));
		pickup->setPosition(sf::Vector2f(random_x, random_y));
		pickup->SetVelocity(0.f, 0.f);

		m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(pickup));
	}
}

int World::GetPlayer1Kills() const
{
	return m_player1_kills;
}

int World::GetPlayer2Kills() const
{
	return m_player2_kills;
}

void World::IncrementPlayer1Kills()
{
	m_player1_kills++;
}

void World::IncrementPlayer2Kills()
{
	m_player2_kills++;
}
