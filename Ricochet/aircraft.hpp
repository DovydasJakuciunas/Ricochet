#pragma once
#include "entity.hpp"
#include "aircraft_type.hpp"
#include "resource_identifiers.hpp"
#include "text_node.hpp"
#include "projectile_type.hpp"
#include "command_queue.hpp"
#include "animation.hpp"
#include <cmath>
#include "utility.hpp"

class Aircraft : public Entity
{
public:
	Aircraft(AircraftType type, const TextureHolder& textures, const FontHolder& fonts);
	unsigned int GetCategory() const override;

	void IncreaseFireRate();
	void IncreaseFireSpread();
	void CollectMissile(unsigned int count);

	void UpdateTexts();

	float GetMaxSpeed() const;
	void Fire();
	void LaunchMissile();
	void CreateBullet(SceneNode& node, const TextureHolder& textures);
	void CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures);
	sf::Vector2f GetBulletSpawnPosition(float x_offset) const;

	sf::FloatRect GetBoundingRect() const override;
	bool IsMarkedForRemoval() const override;
	void PlayLocalSound(CommandQueue& commands, SoundEffect effect);

	void IncrementForwardTime(sf::Time dt);
	void ResetForwardTime();
	sf::Time GetForwardAccelerationTime() const;
	void IncrementReleaseTime(sf::Time dt);
	void ResetReleaseTime();
	sf::Time GetReleaseTime() const;
	void StoreVelocityAtRelease();
	sf::Vector2f GetVelocityAtRelease() const;
	void InvertVelocityX();
	void InvertVelocityY();
	void InvertRotation();
	void AlignVelocityToRotation();
	void AlignRotationToDirection();

private:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

	void CheckProjectileLaunch(sf::Time dt, CommandQueue& commands);
	bool IsAllied() const;
	void CreatePickup(SceneNode& node, const TextureHolder& textures);
	void CheckPickupDrop(CommandQueue& commands);
	void UpdateRollAnimation();

private:
	AircraftType m_type;
	sf::Sprite m_sprite;
	Animation m_explosion;

	TextNode* m_health_display;
	TextNode* m_missile_display;

	float m_distance_travelled;
	int m_directions_index;

	Command m_fire_command;
	Command m_missile_command;
	Command m_drop_pickup_command;

	unsigned int m_fire_rate;
	unsigned int m_spread_level;
	unsigned int m_missile_ammo;

	bool m_is_firing;
	bool m_is_launching_missile;
	bool m_spawned_pickup;


	sf::Time m_fire_countdown;

	bool m_is_marked_for_removal;
	bool m_show_explosion;
	bool m_explosion_began;

	sf::Time m_forward_acceleration_time;
	sf::Time m_release_time;
	sf::Vector2f m_velocity_at_release;

};

