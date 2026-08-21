#pragma once
#include "texture_id.hpp"
#include "fontID.hpp"
#include "resource_holder.hpp"
#include <SFML/Graphics/Font.hpp>

namespace sf
{
	class Texture;
}

//template<typename Identifier, typename Resource>

typedef ResourceHolder<TextureID, sf::Texture> TextureHolder;
typedef ResourceHolder<FontID, sf::Font> FontHolder;
