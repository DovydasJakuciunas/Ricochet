#pragma once
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

class Aircraft;
class TextNode;

class GameplayManager
{
public:
	GameplayManager(Aircraft* player1, Aircraft* player2, TextNode* kill_display1, TextNode* kill_display2);

	// Kill tracking
	int GetPlayer1Kills() const;
	int GetPlayer2Kills() const;
	void IncrementPlayer1Kills();
	void IncrementPlayer2Kills();

	// Player state tracking
	void Update(Aircraft* player1, Aircraft* player2);
	void RespawnDeadPlayers(const sf::Vector2f& spawn_pos_p1, const sf::Vector2f& spawn_pos_p2);

private:
	// Kill counts
	int m_player1_kills;
	int m_player2_kills;

	// Player alive status tracking (for detecting transitions)
	bool m_player1_was_alive;
	bool m_player2_was_alive;

	// UI references
	TextNode* m_player1_kill_display;
	TextNode* m_player2_kill_display;

	// Helper methods
	void UpdateKillDisplay(TextNode* display, int kill_count);
};
