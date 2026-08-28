#include "menu_options.hpp"
#include "fontID.hpp"
#include <SFML/Graphics/Text.hpp>
#include "utility.hpp"
#include "menu_options.hpp"

MenuState::MenuState(StateStack& stack, Context context) : State(stack, context), m_background_sprite(context.textures->Get(TextureID::kTitleScreen)), m_option_index(0)
{
    sf::Font font = context.fonts->Get(FontID::kMain);
    sf::Text play_option(font);
    play_option.setString("Play");
    Utility::CentreOrigin(play_option);
    play_option.setPosition(context.window->getView().getSize() / 2.f);
    m_options.emplace_back(play_option);

    sf::Text exit_option(font);
    play_option.setString("Exit");
    Utility::CentreOrigin(exit_option);
    play_option.setPosition(play_option.getPosition() + sf::Vector2f(0.f, 30.f));
    m_options.emplace_back(exit_option);

    UpdateOptionText();
}

void MenuState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());
    window.draw(m_background_sprite);
    for (const sf::Text& text : m_options)
    {
        window.draw(text);
    }
}

bool MenuState::Update(sf::Time dt)
{
    return true;
}

bool MenuState::HandleEvent(const sf::Event& event)
{
    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (!key_pressed)
    {
        return false;
    }
    if (key_pressed->scancode == sf::Keyboard::Scancode::Enter)
    {
        if (m_option_index == static_cast<int>(MenuOptions::kPlay))
        {
            RequestStackPop();
            RequestStackPush(StateID::kGame);
        }
        else if (m_option_index == static_cast<int>(MenuOptions::kExit))
        {
            RequestStackPop();
        }
    }
    else if (key_pressed->scancode == sf::Keyboard::Scancode::Up)
    {
        if (m_option_index > 0)
        {
            m_option_index--;
        }
        else
        {
            m_option_index = m_options.size() - 1;
        }
        UpdateOptionText();
    }
    else if (key_pressed->scancode == sf::Keyboard::Scancode::Down)
    {
        if (m_option_index < m_options.size() - 1)
        {
            m_option_index++;
        }
        else
        {
            m_option_index = 0;
        }
        UpdateOptionText();
    }
    return true;
}

void MenuState::UpdateOptionText()
{
    if (m_options.empty())
    {
        return;
    }
    for (sf::Text& text : m_options)
    {
        text.setFillColor(sf::Color::White);
    }
    m_options[m_option_index].setFillColor(sf::Color::Red);
}