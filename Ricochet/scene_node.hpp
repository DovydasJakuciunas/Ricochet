#pragma once
#include <SFML/Graphics.hpp>
#include "receiver_categories.hpp"
#include "command.hpp"


class SceneNode : public sf::Transformable, public sf::Drawable
{
public:
	typedef std::unique_ptr<SceneNode> Ptr;

public:
	SceneNode();
	void AttachChild(Ptr child);
	Ptr DetachChild(const SceneNode& node);

	void Update(const sf::Time& dt);

	sf::Vector2f GetWorldPosition() const;
	sf::Transform GetWorldTransform() const;

	virtual void OnCommand(const Command& command, sf::Time dt);

private:
	virtual void UpdateCurrent(const sf::Time& dt);
	void UpdateChildren(const sf::Time& dt);

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	void DrawChildren(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual unsigned int GetCategory() const;

private:
	std::vector<Ptr> m_children;
	SceneNode* m_parent;
};