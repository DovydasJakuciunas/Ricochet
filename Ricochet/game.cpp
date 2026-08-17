#include "game.hpp"
#include "utility.hpp"
#include "constants.hpp"
#include "scene_node.hpp"

Game::Game()
	: m_window(sf::VideoMode({ 640, 480 }), "SFML Refactor"), m_world(m_window)
{
	m_textures.Load(TextureID::kAlphaPlayer, "Media/Textures/AlphaPlayer.png");
	m_player = std::make_unique<sf::Sprite>(m_textures.Get(TextureID::kAlphaPlayer));

	// Set origin to center of sprite for symmetric rotation
	sf::FloatRect bounds = m_player->getLocalBounds();
	m_player->setOrigin(bounds.getCenter());

	m_player->setPosition({ 100.f, 100.f });
}

void Game::Run()
{
	sf::Clock clock;
	sf::Time time_since_last_update = sf::Time::Zero;
	while (m_window.isOpen())
	{
		time_since_last_update += clock.restart();
		while (time_since_last_update.asSeconds() > kTimePerFrame)
		{
			time_since_last_update -= sf::seconds(kTimePerFrame);
			ProcessInputs();
			Update(sf::seconds(kTimePerFrame));
		}
		Render();
	}
}

void Game::ProcessInputs()
{
	CommandQueue & compl = m_world.GetCommandQueue();

	while (const std::optional event = m_window.pollEvent())
	{
		m_player.HandleEvents(event, commands);

		if (event->is<sf::Event::Closed>())
		{
			m_window.close();
			break;
		}
	}
	m_player.HandleRealTimeInput(commands);
}

void Game::Update(sf::Time delta_time)
{
	m_world.Update(delta_time);

	//TODO - Move this elsewhere
	/*
	const float dt = delta_time.asSeconds();

	// Handle Speed of Player
	if (m_is_accelerating)
	{
		m_current_speed = std::min(m_current_speed + kPlayerAcceleration * dt, kPlayerSpeed);
	}
	else if (m_is_decelerating)
	{
		m_current_speed = std::max(m_current_speed - kPlayerDeceleration * dt, 0.f);
	}
	else
	{
		// Physics based drag: play starts losing speed if not accelerating
		float drag_deceleration = (drag_coefficient * m_current_speed * m_current_speed) / kPlayerSpeed;
		m_current_speed = std::max(m_current_speed - drag_deceleration * dt, 0.f);
	}

	// Handle Rotation Speed
	if (m_is_rotating_left)
	{
		m_rotation -= kPlayerRotationSpeed * dt;
	}
	if (m_is_rotating_right)
	{
		m_rotation += kPlayerRotationSpeed * dt;
	}

	m_player->setRotation(sf::degrees(m_rotation + 180));

	//Starting at 90 for SFML, Calculating Players angle, converts to unit vector and moves the player
	const float angle_rad = (m_rotation + 90.f) * kPieValue / 180.f;
	sf::Vector2f direction(std::cos(angle_rad), std::sin(angle_rad));
	m_player->move(direction * m_current_speed * dt);
	*/
}

void Game::Render()
{
	m_window.clear();
	m_window.Draw();
	m_window.display();
}


//TODO - Also Move this elsewhere

/*
void Game::HandlePlayerInput(sf::Keyboard::Scancode key, bool is_pressed)
{
	if (key == sf::Keyboard::Scancode::W)
	{
		m_is_accelerating = is_pressed;
	}
	if (key == sf::Keyboard::Scancode::S)
	{
		m_is_decelerating = is_pressed;
	}
	if (key == sf::Keyboard::Scancode::A)
	{
		m_is_rotating_left = is_pressed;
	}
	if (key == sf::Keyboard::Scancode::D)
	{
		m_is_rotating_right = is_pressed;
	}
	*/
}