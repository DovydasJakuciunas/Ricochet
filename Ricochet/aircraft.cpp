#include "aircraft.hpp"
#include "weapon_system.hpp"
#include "movement_controller.hpp"
#include "texture_id.hpp"
#include "data_tables.hpp"
#include "constants.hpp"
#include "utility.hpp"


namespace
{
	const std::vector<AircraftData> Table = InitializeAircraftData();

	// Explosion animation constants
	const int kExplosionFrameSize = 256;
	const int kExplosionFrameCount = 16;
	const float kExplosionDuration = 1.0f;
}

TextureID ToTextureID(AircraftType type)
{
	switch (type)
	{
	case AircraftType::kEagle:
		return TextureID::kEagle;
		break;
	case AircraftType::kRaptor:
		return TextureID::kRaptor;
		break;
	}
	return TextureID::kEagle;
}

Aircraft::Aircraft(AircraftType type, const TextureHolder& textures, const FontHolder& fonts)
	: Entity(Table[static_cast<int>(type)].m_hitpoints)
	, m_type(type)
	, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect)
	, m_health_display(nullptr)
	, m_missile_display(nullptr)
	, m_distance_travelled(0.f)
	, m_directions_index(0)
	, m_is_marked_for_removal(false)
	, m_show_explosion(true)
	, m_explosion(textures.Get(TextureID::kExplosion))
	, m_explosion_began(false)
	, m_weapon_system(std::make_unique<WeaponSystem>(this, textures))
	, m_movement_controller(std::make_unique<MovementController>(this))
{
	m_explosion.SetFrameSize(sf::Vector2i(kExplosionFrameSize, kExplosionFrameSize));
	m_explosion.SetNumFrames(kExplosionFrameCount);
	m_explosion.SetDuration(sf::seconds(kExplosionDuration));
	Utility::CentreOrigin(m_sprite);
	Utility::CentreOrigin(m_explosion);

	std::string* health = new std::string("");
	std::unique_ptr<TextNode> health_display(new TextNode(fonts, *health));
	m_health_display = health_display.get();
	AttachChild(std::move(health_display));

	if (Aircraft::GetCategory() == static_cast<int>(ReceiverCategories::kPlayerAircraft))
	{
		std::string* missile_ammo = new std::string("");
		std::unique_ptr<TextNode> missile_display(new TextNode(fonts, *missile_ammo));
		m_missile_display = missile_display.get();
		AttachChild(std::move(missile_display));
	}
	UpdateTexts();
}

Aircraft::~Aircraft()
{
}

unsigned int Aircraft::GetCategory() const
{
	if (IsAllied())
	{
		return static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft);
	}
	return static_cast<unsigned int>(ReceiverCategories::kEnemyAircraft);
}

void Aircraft::IncreaseFireRate()
{
	m_weapon_system->IncreaseFireRate();
}

void Aircraft::IncreaseFireSpread()
{
	m_weapon_system->IncreaseFireSpread();
}

void Aircraft::CollectMissile(unsigned int count)
{
	m_weapon_system->CollectMissile(count);
}

void Aircraft::UpdateTexts()
{
	m_health_display->SetString(std::to_string(GetHitPoints()) + "HP");
	m_health_display->setPosition(sf::Vector2f(0.f, 50.f));
	m_health_display->setRotation(-getRotation());

	if (m_missile_display)
	{
		m_missile_display->setPosition(sf::Vector2f(0.f, 70.f));
		if (m_weapon_system->GetMissileAmmo() == 0)
		{
			m_missile_display->SetString("");
		}
		else
		{
			m_missile_display->SetString("M: " + std::to_string(m_weapon_system->GetMissileAmmo()));
		}
	}
}

float Aircraft::GetMaxSpeed() const
{
	return Table[static_cast<int>(m_type)].m_speed;
}

void Aircraft::Fire()
{
	m_weapon_system->Fire();
}

void Aircraft::LaunchMissile()
{
	m_weapon_system->LaunchMissile();
}

void Aircraft::CreateBullet(SceneNode& node, const TextureHolder& textures)
{
	m_weapon_system->CreateBullet(node, textures);
}

void Aircraft::CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures)
{
	m_weapon_system->CreateProjectile(node, type, x_offset, y_offset, textures);
}

sf::Vector2f Aircraft::GetBulletSpawnPosition(float x_offset) const
{
	return m_weapon_system->GetBulletSpawnPosition(x_offset);
}

sf::FloatRect Aircraft::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

bool Aircraft::IsMarkedForRemoval() const
{
	return IsDestroyed() && (m_explosion.IsFinished() || !m_show_explosion);
}

void Aircraft::PlayLocalSound(CommandQueue& commands, SoundEffect effect)
{
	m_weapon_system->PlayLocalSound(commands, effect);
}

void Aircraft::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (IsDestroyed() && m_show_explosion)
	{
		target.draw(m_explosion, states);
	}
	else
	{
		target.draw(m_sprite, states);
	}
}

void Aircraft::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	if (IsDestroyed())
	{
		m_explosion.Update(dt);
		//Play explosion sound only once
		if (!m_explosion_began)
		{
			SoundEffect soundEffect = (Utility::RandomInt(2) == 0) ? SoundEffect::kExplosion1 : SoundEffect::kExplosion2;
			PlayLocalSound(commands, soundEffect);
			m_explosion_began = true;
		}
		m_is_marked_for_removal = true;
		return;
	}

	Entity::UpdateCurrent(dt, commands);
	UpdateTexts();

	UpdateRollAnimation();

	// Update weapon system
	m_weapon_system->Update(dt, commands);
}

bool Aircraft::IsAllied() const
{
	return m_type == AircraftType::kEagle;
}

void Aircraft::UpdateRollAnimation()
{
	if (Table[static_cast<int>(m_type)].m_has_roll_animation)
	{
		sf::IntRect textureRect = Table[static_cast<int>(m_type)].m_texture_rect;

		//Roll left: Texture rect is offset once
		if (GetVelocity().x < 0.f)
		{
			textureRect.position.x += textureRect.size.x;
		}
		else if (GetVelocity().x > 0.f)
		{
			textureRect.position.x += 2 * textureRect.size.x;
		}
		m_sprite.setTextureRect(textureRect);

	}
}

void Aircraft::IncrementForwardTime(sf::Time dt)
{
	m_movement_controller->IncrementForwardTime(dt);
}

void Aircraft::ResetForwardTime()
{
	m_movement_controller->ResetForwardTime();
}

sf::Time Aircraft::GetForwardAccelerationTime() const
{
	return m_movement_controller->GetForwardAccelerationTime();
}

void Aircraft::IncrementReleaseTime(sf::Time dt)
{
	m_movement_controller->IncrementReleaseTime(dt);
}

void Aircraft::ResetReleaseTime()
{
	m_movement_controller->ResetReleaseTime();
}

sf::Time Aircraft::GetReleaseTime() const
{
	return m_movement_controller->GetReleaseTime();
}

void Aircraft::StoreVelocityAtRelease()
{
	m_movement_controller->StoreVelocityAtRelease();
}

sf::Vector2f Aircraft::GetVelocityAtRelease() const
{
	return m_movement_controller->GetVelocityAtRelease();
}

void Aircraft::InvertVelocityX()
{
	m_movement_controller->InvertVelocityX();
}

void Aircraft::InvertVelocityY()
{
	m_movement_controller->InvertVelocityY();
}

void Aircraft::InvertRotation()
{
	m_movement_controller->InvertRotation();
}

void Aircraft::AlignVelocityToRotation()
{
	m_movement_controller->AlignVelocityToRotation();
}

void Aircraft::AlignRotationToDirection()
{
	m_movement_controller->AlignRotationToDirection();
}
