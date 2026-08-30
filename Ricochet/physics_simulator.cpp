#include "physics_simulator.hpp"
#include "aircraft.hpp"
#include "scene_node.hpp"
#include "entity.hpp"
#include "action.hpp"
#include "receiver_categories.hpp"
#include <cmath>

PhysicsSimulator::PhysicsSimulator(const sf::FloatRect& world_bounds, const sf::View& camera)
	: m_world_bounds(world_bounds)
	, m_camera(camera)
{
}

sf::FloatRect PhysicsSimulator::GetViewBounds() const
{
	return sf::FloatRect(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());
}

sf::FloatRect PhysicsSimulator::GetBattleFieldBounds() const
{
	// Return camera bounds + a small area off screen where the enemies spawn
	sf::FloatRect bounds = GetViewBounds();
	bounds.position.y -= 100.f;
	bounds.size.y += 100.f;
	return bounds;
}

void PhysicsSimulator::HandlePlayerBoundaryCollision(Aircraft* player1, Aircraft* player2)
{
	// Keep player within the camera view bounds with a border
	sf::FloatRect view_bounds = GetViewBounds();
	const float border_distance = 40.f;

	// Handle Player 1 collision
	if (player1)
	{
		sf::Vector2f position = player1->getPosition();
		sf::FloatRect player_bounds = player1->GetBoundingRect();

		// Keep player within bounds (invert velocity and rotation on wall collision)
		if (player_bounds.position.x <= view_bounds.position.x + border_distance)
		{
			// Hit left boundary - invert X velocity and rotation
			position.x = view_bounds.position.x + border_distance + (player_bounds.size.x / 2.f);
			player1->InvertVelocityX();
			player1->InvertRotation();
		}
		else if (player_bounds.position.x + player_bounds.size.x >= view_bounds.position.x + view_bounds.size.x - border_distance)
		{
			// Hit right boundary - invert X velocity and rotation
			position.x = view_bounds.position.x + view_bounds.size.x - border_distance - (player_bounds.size.x / 2.f);
			player1->InvertVelocityX();
			player1->InvertRotation();
		}

		// Keep player within bounds vertically
		if (player_bounds.position.y <= view_bounds.position.y + border_distance)
		{
			// Hit top boundary - invert Y velocity and rotation
			position.y = view_bounds.position.y + border_distance + (player_bounds.size.y / 2.f);
			player1->InvertVelocityY();
			player1->InvertRotation();
		}
		else if (player_bounds.position.y + player_bounds.size.y >= view_bounds.position.y + view_bounds.size.y - border_distance)
		{
			// Hit bottom boundary - invert Y velocity and rotation
			position.y = view_bounds.position.y + view_bounds.size.y - border_distance - (player_bounds.size.y / 2.f);
			player1->InvertVelocityY();
			player1->InvertRotation();
		}

		player1->setPosition(position);
	}

	// Handle Player 2 collision (same logic)
	if (player2)
	{
		sf::Vector2f position = player2->getPosition();
		sf::FloatRect player_bounds = player2->GetBoundingRect();

		// Keep player within bounds (invert velocity and rotation on wall collision)
		if (player_bounds.position.x <= view_bounds.position.x + border_distance)
		{
			// Hit left boundary - invert X velocity and rotation
			position.x = view_bounds.position.x + border_distance + (player_bounds.size.x / 2.f);
			player2->InvertVelocityX();
			player2->InvertRotation();
		}
		else if (player_bounds.position.x + player_bounds.size.x >= view_bounds.position.x + view_bounds.size.x - border_distance)
		{
			// Hit right boundary - invert X velocity and rotation
			position.x = view_bounds.position.x + view_bounds.size.x - border_distance - (player_bounds.size.x / 2.f);
			player2->InvertVelocityX();
			player2->InvertRotation();
		}

		// Keep player within bounds vertically
		if (player_bounds.position.y <= view_bounds.position.y + border_distance)
		{
			// Hit top boundary - invert Y velocity and rotation
			position.y = view_bounds.position.y + border_distance + (player_bounds.size.y / 2.f);
			player2->InvertVelocityY();
			player2->InvertRotation();
		}
		else if (player_bounds.position.y + player_bounds.size.y >= view_bounds.position.y + view_bounds.size.y - border_distance)
		{
			// Hit bottom boundary - invert Y velocity and rotation
			position.y = view_bounds.position.y + view_bounds.size.y - border_distance - (player_bounds.size.y / 2.f);
			player2->InvertVelocityY();
			player2->InvertRotation();
		}

		player2->setPosition(position);
	}
}

void PhysicsSimulator::BounceProjectiles(CommandQueue& command_queue)
{
	// Apply bouncing to all projectiles in the scene
	Command projectileBouncer;
	projectileBouncer.category = static_cast<int>(ReceiverCategories::kProjectile);
	projectileBouncer.action = DerivedAction<SceneNode>([this](SceneNode& node, sf::Time)
		{
			BounceEntity(&node);
		});
	command_queue.Push(projectileBouncer);
}

void PhysicsSimulator::BounceEntity(SceneNode* entity)
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
		// Hit top wall - bounce back down
		velocity.y = std::abs(velocity.y);
		position.y = m_world_bounds.position.y + (bounds.size.y / 2.f);
		bounced = true;
	}
	else if (bounds.position.y + bounds.size.y >= m_world_bounds.position.y + m_world_bounds.size.y)
	{
		// Hit bottom wall - bounce back up
		velocity.y = -std::abs(velocity.y);
		position.y = m_world_bounds.position.y + m_world_bounds.size.y - (bounds.size.y / 2.f);
		bounced = true;
	}

	// Apply bounced velocity and position
	if (bounced)
	{
		moving_entity->SetVelocity(velocity);
		entity->setPosition(position);
	}
}
