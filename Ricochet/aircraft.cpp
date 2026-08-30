#include "aircraft.hpp"
#include "texture_id.hpp"
#include "data_tables.hpp"
#include "constants.hpp"
#include "projectile.hpp"
#include "projectile_type.hpp"
#include "sound_node.hpp"


namespace
{
	const std::vector<AircraftData> Table = InitializeAircraftData();
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
	, m_fire_rate(1)
	, m_spread_level(1)
	, m_is_firing(false)
	, m_is_launching_missile(false)
	, m_fire_countdown(sf::Time::Zero)
	, m_missile_ammo(2)
	, m_is_marked_for_removal(false)
	, m_show_explosion(true)
	, m_explosion(textures.Get(TextureID::kExplosion))
	, m_explosion_began(false)
	, m_forward_acceleration_time(sf::Time::Zero)
	, m_release_time(sf::Time::Zero)
	, m_velocity_at_release(0.f, 0.f)
{
	m_explosion.SetFrameSize(sf::Vector2i(256, 256));
	m_explosion.SetNumFrames(16);
	m_explosion.SetDuration(sf::seconds(1));
	Utility::CentreOrigin(m_sprite);
	Utility::CentreOrigin(m_explosion);

	m_fire_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_fire_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{
			CreateBullet(node, textures);
		};

	m_missile_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_missile_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{
			CreateProjectile(node, ProjectileType::kMissile, 0.f, 0.5f, textures);
		};

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
	if (m_fire_rate < kMaxFireRate)
	{
		++m_fire_rate;
	}
}

void Aircraft::IncreaseFireSpread()
{
	if (m_spread_level < kMaxSpread)
	{
		++m_spread_level;
	}
}

void Aircraft::CollectMissile(unsigned int count)
{
	m_missile_ammo += count;
}

void Aircraft::UpdateTexts()
{
	m_health_display->SetString(std::to_string(GetHitPoints()) + "HP");
	m_health_display->setPosition(sf::Vector2f(0.f, 50.f));
	m_health_display->setRotation(-getRotation());

	if (m_missile_display)
	{
		m_missile_display->setPosition(sf::Vector2f(0.f, 70.f));
		if (m_missile_ammo == 0)
		{
			m_missile_display->SetString("");
		}
		else
		{
			m_missile_display->SetString("M: " + std::to_string(m_missile_ammo));
		}
	}
}

float Aircraft::GetMaxSpeed() const
{
	return Table[static_cast<int>(m_type)].m_speed;
}

void Aircraft::Fire()
{
	if (Table[static_cast<int>(m_type)].m_fire_interval != sf::Time::Zero)
	{
		m_is_firing = true;
	}
}

void Aircraft::LaunchMissile()
{
	if (m_missile_ammo > 0)
	{
		m_is_launching_missile = true;
		--m_missile_ammo;
	}
}

void Aircraft::CreateBullet(SceneNode& node, const TextureHolder& textures)
{
	ProjectileType type = IsAllied() ? ProjectileType::kAlliedBullet : ProjectileType::kEnemyBullet;
	switch (m_spread_level)
	{
	case 1:
		if (m_is_firing)
		{
			CreateProjectile(node, type, 0.0f, 0.5f, textures);
			m_is_firing = false;
		}
		break;
	case 2:
		if (m_is_firing)
		{
			CreateProjectile(node, type, -0.5f, 0.5f, textures);
			CreateProjectile(node, type, 0.5f, 0.5f, textures);
			m_is_firing = false;
			break;
		}
	case 3:
		if (m_is_firing)
		{
			CreateProjectile(node, type, 0.0f, 0.5f, textures);
			CreateProjectile(node, type, -0.5f, 0.5f, textures);
			CreateProjectile(node, type, 0.5f, 0.5f, textures);
			m_is_firing = false;
			break;
		}
	}
}

void Aircraft::CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures)
{
	if (m_is_launching_missile || m_is_firing)
	{
		std::unique_ptr<Projectile> projectile(new Projectile(type, textures));
		sf::Vector2f spawnPosition = GetBulletSpawnPosition(x_offset);

		// Calculate velocity in the direction the jet is facing
		float rotationDegrees = getRotation().asDegrees();
		double rotationRadians = Utility::toRadians(rotationDegrees + -90.f);
		float maxSpeed = projectile->GetMaxSpeed();

		float dirX = -std::cos(rotationRadians);
		float dirY = -std::sin(rotationRadians);

		float sign = IsAllied() ? -1.f : 1.f;
		sf::Vector2f velocity(dirX * maxSpeed * sign, dirY * maxSpeed * sign);

		projectile->setPosition(spawnPosition);
		projectile->SetVelocity(velocity);
		node.AttachChild(std::move(projectile));
		m_is_launching_missile = false;
	}
}

sf::Vector2f Aircraft::GetBulletSpawnPosition(float x_offset) const
{
	// Get the jet's bounding box and center
	sf::FloatRect bounds = m_sprite.getGlobalBounds();
	sf::Vector2f jetCenter = GetWorldPosition();

	// Calculate the front (nose) of the jet based on rotation
	// The jet's front is perpendicular to its body, pointing in the direction it's facing
	float rotationDegrees = getRotation().asDegrees();
	double rotationRadians = Utility::toRadians(rotationDegrees + 90.f);

	// Distance from center to front of jet (half height)
	float frontDistance = bounds.size.y * 0.5f;

	// Calculate front position based on rotation
	float frontX = -std::cos(rotationRadians) * frontDistance;
	float frontY = -std::sin(rotationRadians) * frontDistance;

	// Apply horizontal spread offset at the front
	float spreadOffsetX = x_offset * bounds.size.x;
	double spreadRotationRadians = Utility::toRadians(rotationDegrees);
	float perpX = -std::cos(spreadRotationRadians) * spreadOffsetX;
	float perpY = -std::sin(spreadRotationRadians) * spreadOffsetX;

	sf::Vector2f spawnPos = jetCenter + sf::Vector2f(frontX + perpX, frontY + perpY);
	return spawnPos;
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
	sf::Vector2f world_position = GetWorldPosition();

	Command command;
	command.category = static_cast<int>(ReceiverCategories::kSoundEffect);
	command.action = DerivedAction<SoundNode>(
		[effect, world_position](SoundNode& node, sf::Time)
		{
			node.PlaySound(effect, world_position);
		});
	commands.Push(command);
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

	//Check if bullets or missiles were fired
	CheckProjectileLaunch(dt, commands);
}

void Aircraft::CheckProjectileLaunch(sf::Time dt, CommandQueue& commands)
{
	if (!IsAllied())
	{
		Fire();
	}

	if (m_is_firing && m_fire_countdown <= sf::Time::Zero)
	{
		PlayLocalSound(commands, IsAllied() ? SoundEffect::kEnemyGunfire : SoundEffect::kAlliedGunfire);
		commands.Push(m_fire_command);
		m_fire_countdown += Table[static_cast<int>(m_type)].m_fire_interval / (m_fire_rate + 1.f);
	}
	else if (m_fire_countdown > sf::Time::Zero)
	{
		m_fire_countdown -= dt;
		m_is_firing = false;
	}

	//Missile launch
	if (m_is_launching_missile)
	{
		PlayLocalSound(commands, SoundEffect::kLaunchMissile);
		commands.Push(m_missile_command);
		//m_is_launching_missile = false;
	}
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
	m_forward_acceleration_time += dt;
}

void Aircraft::ResetForwardTime()
{
	m_forward_acceleration_time = sf::Time::Zero;
}

sf::Time Aircraft::GetForwardAccelerationTime() const
{
	return m_forward_acceleration_time;
}

void Aircraft::IncrementReleaseTime(sf::Time dt)
{
	m_release_time += dt;
}

void Aircraft::ResetReleaseTime()
{
	m_release_time = sf::Time::Zero;
}

sf::Time Aircraft::GetReleaseTime() const
{
	return m_release_time;
}

void Aircraft::StoreVelocityAtRelease()
{
	m_velocity_at_release = GetVelocity();
}

sf::Vector2f Aircraft::GetVelocityAtRelease() const
{
	return m_velocity_at_release;
}

void Aircraft::InvertVelocityX()
{
	sf::Vector2f velocity = GetVelocity();
	SetVelocity(-velocity.x, velocity.y);
}

void Aircraft::InvertVelocityY()
{
	sf::Vector2f velocity = GetVelocity();
	SetVelocity(velocity.x, -velocity.y);
}

void Aircraft::InvertRotation()
{
	// Get current rotation and invert it by adding 180 degrees
	float currentRotation = getRotation().asDegrees();
	float invertedRotation = currentRotation + 180.f;

	// Normalize to 0-360 range
	if (invertedRotation >= 360.f)
	{
		invertedRotation -= 360.f;
	}

	setRotation(sf::degrees(invertedRotation));
}

void Aircraft::AlignVelocityToRotation()
{
	// Get current velocity magnitude (speed)
	sf::Vector2f currentVelocity = GetVelocity();
	float speed = std::sqrt(currentVelocity.x * currentVelocity.x + currentVelocity.y * currentVelocity.y);

	if (speed < 0.1f)
	{
		return; // No significant velocity
	}

	// Get rotation angle and convert to radians
	float rotationDegrees = getRotation().asDegrees();
	double radians = Utility::toRadians(rotationDegrees + 90.f);

	// Calculate direction vectors based on rotation
	// Aircraft forward is at 90 degrees in SFML coordinate system
	float dirX = -std::cos(radians);
	float dirY = -std::sin(radians);

	// Apply new velocity in the direction the aircraft is facing, maintaining speed
	SetVelocity(dirX * speed, dirY * speed);
}

void Aircraft::AlignRotationToDirection()
{
	sf::Vector2f velocity = GetVelocity();
	if (velocity.x == 0.f && velocity.y == 0.f)
	{
		return; // No velocity, can't align
	}

	// Calculate angle from velocity vector
	// atan2(y, x) gives angle in radians
	float angle = std::atan2(velocity.y, velocity.x) * 180.f / 3.14159265f;
	// Aircraft forward is at 90 degrees in SFML, so adjust
	angle = angle - 90.f;

	setRotation(sf::degrees(angle));
}
