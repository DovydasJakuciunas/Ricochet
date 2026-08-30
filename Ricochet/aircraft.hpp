#pragma once
#include "entity.hpp"
#include "aircraft_type.hpp"
#include "resource_identifiers.hpp"
#include "text_node.hpp"
#include "projectile_type.hpp"
#include "command_queue.hpp"
#include "animation.hpp"
#include "player.hpp"
#include <memory>
#include <SFML/Window/Keyboard.hpp>

class WeaponSystem;
class MovementController;

class Aircraft : public Entity
{
public:
	Aircraft(AircraftType type, const TextureHolder& textures, const FontHolder& fonts, PlayerID player_id = PlayerID::kPlayer1);
	~Aircraft();
	unsigned int GetCategory() const override;

	// Weapon system delegation
	void IncreaseFireRate();
	void IncreaseFireSpread();
	void CollectMissile(unsigned int count);
	void Fire();
	void LaunchMissile();

	void UpdateTexts();

	float GetMaxSpeed() const;
	sf::FloatRect GetBoundingRect() const override;
	bool IsMarkedForRemoval() const override;
	void PlayLocalSound(CommandQueue& commands, SoundEffect effect);

	// Projectile methods (delegated to WeaponSystem)
	void CreateBullet(SceneNode& node, const TextureHolder& textures);
	void CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures);
	sf::Vector2f GetBulletSpawnPosition(float x_offset) const;

	// Movement controller delegation
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

	// Player ID accessors
	PlayerID GetPlayerID() const;
	void SetPlayerID(PlayerID player_id);

	// Collision immunity grace period
	void SetCollisionImmunity(sf::Time duration);
	bool IsCollisionImmune() const;

	// For subsystem access
	friend class WeaponSystem;
	friend class MovementController;

private:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

	void UpdateRollAnimation();
	bool IsAllied() const;

private:
	AircraftType m_type;
	sf::Sprite m_sprite;
	Animation m_explosion;

	TextNode* m_health_display;
	TextNode* m_missile_display;

	float m_distance_travelled;
	int m_directions_index;

	bool m_is_marked_for_removal;
	bool m_show_explosion;
	bool m_explosion_began;

	// Player identification
	PlayerID m_player_id;

	// Collision immunity grace period
	sf::Time m_collision_immunity_remaining;

	// Subsystems
	std::unique_ptr<WeaponSystem> m_weapon_system;
	std::unique_ptr<MovementController> m_movement_controller;

};

