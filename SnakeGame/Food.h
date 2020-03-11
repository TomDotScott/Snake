#pragma once
#include "SFML/Graphics.hpp"
#include "Entity.h"
#include "Constants.h"


enum class eFoodType {
	e_standard, e_special, e_gobble
};

class Food : public Entity
{
private:
	sf::CircleShape m_circle;

	int RandomRange(int min, int max);
	
	eFoodType m_type;
	
	void RandomisePosition();

public:
	Food();

	void Randomise();


	void Render(sf::RenderWindow& window) override;

	Food(sf::Color colour, sf::Vector2f position);

	sf::Vector2f GetPosition() const { return m_position; }

	int GetGrowAmount() const
	{
		switch (m_type)
		{
		case eFoodType::e_standard:
			return Constants::k_standardGrowAmount;
		case eFoodType::e_special:
			return Constants::k_specialGrowAmount;
		case eFoodType::e_gobble:
			return Constants::k_gobbleGrowAmount;
		}
		return 0;
	}
};


