#include "State_Game.h"

#include <fstream>
#include <iostream>

#include "State_GameOver.h"

/* TO DO
IMPROVE PATHFINDING
	*MAKE SNAKES CHOOSE DESIRED FOOD BASED ON POINT VALUE
	*MAKE SNAKES AWARE OF OTHER SNAKES
		*MAKE SNAKES AWARE OF OTHER SNAKES' HEADS
		*MAKE SNAKES AWARE OF OTHER SNAKES' BODIES
	*MAKE SNAKES AWARE OF THEMSELVES
	*
*/

//BASESTATE METHODS
void State_Game::Initialize(sf::RenderWindow* _window, sf::Font* _font)
{
	m_font = *_font;

	auto* playerSnake = new PlayerSnake();
	m_snakes.push_back(playerSnake);

	//populate the food array
	for (auto& i : m_foodArray) {
		Food* food = new Food();
		i = food;
	}

	
	//populate the snake Vector
	for (int i = 0; i < Constants::k_AISnakeAmount; ++i) {
		std::cout << "AI SNAKE CREATED" << std::endl;
		m_snakes.push_back(new AISnake());
	}
	
	
	//populate the score UI
	for (unsigned int i = 0; i < m_snakes.size(); ++i)
	{
		sf::Text playerText;
		playerText.setFont(m_font);
		std::string textToDisplay = "Player";
		textToDisplay += std::to_string(i + 1) + ":";
		playerText.setString(textToDisplay);
		playerText.setFillColor(sf::Color::White);
		playerText.setCharacterSize(25);

		//Work out where they will be positioned
		playerText.setPosition(sf::Vector2f(static_cast<float>(Constants::k_screenWidth - 175), static_cast<float>((i * Constants::k_gridSize) + 10)));
		m_scores.push_back(playerText);
	}

	//Initialise Gobble mode text
	m_gobbleModeText = new sf::Text("Gobble Mode!", m_font, 20U);
	m_gobbleModeText->setOrigin(m_gobbleModeText->getGlobalBounds().width / 2, m_gobbleModeText->getGlobalBounds().height / 2);
	m_gobbleModeText->setFillColor(sf::Color::Yellow);
	m_gobbleModeText->setPosition(Constants::k_screenWidth - 100, Constants::k_screenHeight - 200);

	//Initialise Pause Text
	m_pausedText = new sf::Text("Paused", m_font, 128U);
	m_pausedText->setOrigin(m_pausedText->getGlobalBounds().width / 2, m_pausedText->getGlobalBounds().height / 2);
	m_pausedText->setFillColor(sf::Color::Red);
	m_pausedText->setPosition(static_cast<float>(Constants::k_screenWidth) / 2.f, static_cast<float>(Constants::k_screenHeight) / 2.f);

	//make the snakes know where the food and other snakes are on the screen
	for (auto* snake : m_snakes)
	{
		for (auto* food : m_foodArray)
		{
			snake->SetFood(food);
		}

		for(auto* otherSnake : m_snakes)
		{
			if(snake != otherSnake)
			{
				snake->SetOtherSnake(otherSnake);
			}
		}
	}
}

void State_Game::Update(sf::RenderWindow* _window) {
	GetInput();
	//only play the game if it is paused
	if (!m_paused) {
		m_gobble = false;

		CheckCollisions();

		for (auto* snake : m_snakes)
		{
			if (snake->GetIsGobbleMode())
			{
				m_gobble = true;
			}
			snake->Update();
		}

		//GOBBLE MODE. After a random amount of time, stop Gobble Mode
		if (rand() % 25 == 0 && m_gobble) {
			for (auto* snake : m_snakes) {
				if (!snake->IsDead() && snake->GetIsGobbleMode()) {
					std::cout << "GOBBLE MODE OVER" << std::endl;
					snake->SetIsGobbleMode(false);
					break;
				}
			}
		}


		UpdateScores();
		//If the player has died, end the game
		if (m_snakes[0]->IsDead())
		{
			SaveScores();
			current_state = eCurrentState::e_GameOver;
			core_state.SetState(new State_GameOver());
		}
	}
}

void State_Game::Render(sf::RenderWindow* _window)
{
	//Render the food
	for (Food* food : m_foodArray) {
		food->Render(*_window);
	}

	//Render the snakes
	for(Snake* snake : m_snakes)
	{
		snake->Render(*_window);
	}

	//Draw the UI
	for (const auto& score : m_scores)
	{
		_window->draw(score);
	}

	if (m_gobble)
	{
		_window->draw(*m_gobbleModeText);
	}
	
	//Draw the Walls
	_window->draw(m_topWall.m_wall);
	_window->draw(m_bottomWall.m_wall);
	_window->draw(m_leftWall.m_wall);
	_window->draw(m_rightWall.m_wall);

	if(m_paused)
	{
		_window->draw(*m_pausedText);
	}
}

void State_Game::Destroy(sf::RenderWindow* _window)
{
	for (Food* food : m_foodArray) {
		delete food;
	}

	for (auto* snake : m_snakes)
	{
		delete snake;
	}
}


//GAME METHODS
void State_Game::CheckCollisions() {
	for (auto* currentSnake : m_snakes) {
		//only check collisions if the snake is alive
		if (!currentSnake->IsDead()) {
			//Check against Walls
			if (currentSnake->GetHeadPosition().x == m_leftWall.m_position.x ||
				currentSnake->GetHeadPosition().x == m_rightWall.m_position.x ||
				currentSnake->GetHeadPosition().y == m_topWall.m_position.y ||
				currentSnake->GetHeadPosition().y == m_bottomWall.m_position.y) {
				currentSnake->Collision(ECollisionType::e_wall);
				return;
			}

			//Check Against Food
			for (auto* food : m_foodArray) {
				if (food->GetPosition() == currentSnake->GetHeadPosition()) {
					currentSnake->Collision(food);
					RandomiseFood(food);
					return;
				}
			}
			
			//Check against other snakes
			for (auto* otherSnake : m_snakes) {
				if (!otherSnake->IsDead() && (otherSnake != currentSnake)) {
					//Check each segment of the current snake against the heads of the other snakes
					auto currentSegment = currentSnake->GetSnakeSegments().GetHead();
					for (int i = 0; i < currentSnake->GetSnakeSegments().Size(); ++i)
					{
						//Check each segment against the heads of the other snakes
						if (currentSegment->m_position == otherSnake->GetHeadPosition())
						{
							//if it's a head on collision then both snakes die
							if (currentSnake->GetHeadPosition() == currentSegment->m_position)
							{
								if (currentSnake->GetIsGobbleMode()) {
									currentSnake->Grow(static_cast<const int>((otherSnake->GetSnakeSegments().Size())));
									otherSnake->Collision(ECollisionType::e_snake);
									return;
								}
								currentSnake->Collision(ECollisionType::e_snake);
								otherSnake->Collision(ECollisionType::e_snake);
								return;
							}
							otherSnake->Collision(ECollisionType::e_snake);
						}
						currentSegment = currentSegment->m_nextNode;
					}
					
					//Check if the current snake has hit another snake's body
					auto otherSegment = otherSnake->GetSnakeSegments().GetHead();

					for (int i = 0; i < otherSnake->GetSnakeSegments().Size(); ++i)
					{
						if(otherSegment->m_position == currentSnake->GetHeadPosition())
						{
							//If it's gobble mode, make sure not to kill the player on collision
							if (currentSnake->GetIsGobbleMode()) {
								const int growShrinkAmount{ otherSnake->FindGobblePoint(currentSnake->GetHeadPosition()) };
								currentSnake->Grow(growShrinkAmount);
								otherSnake->Shrink(growShrinkAmount);
								return;
							}
							else {
								currentSnake->Collision(ECollisionType::e_snake);
								return;
							}
						}
						otherSegment = otherSegment->m_nextNode;
					}
				}
			}
		}
	}
}

void State_Game::RandomiseFood(Food* _foodToRandomise)
{
	//Check that food doesn't spawn on top of each other
	bool isOverlapping = true;
	while (isOverlapping) {
		_foodToRandomise->Randomise();
		//Check the randomised position
		for (const Food* food : m_foodArray) {
			//make sure the food isn't getting compared to itself!
			if (*food != *_foodToRandomise) {
				if (_foodToRandomise->GetPosition() != food->GetPosition()) {
					isOverlapping = false;
					break;
				}
			}
			else {
				_foodToRandomise->Randomise();
			}
		}
	}
}

void State_Game::UpdateScores()
{
	for (unsigned int i = 0; i < m_snakes.size(); ++i)
	{
		std::string textToDisplay = "Player" + std::to_string(i + 1) + ":" + std::to_string(m_snakes[i]->GetScore());
		m_scores[i].setString(textToDisplay);
	}
}

void State_Game::GetInput()
{
	//pause and un-pause the game if escape is pressed
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
	{
		m_paused = !m_paused;
		std::cout << (m_paused ? "PAUSED" : "UNPAUSED") << std::endl;
	}
	//access the player's input function
	for (auto* snake : m_snakes)
	{
		auto* playerSnake = dynamic_cast<PlayerSnake*>(snake);
		if (playerSnake)
		{
			playerSnake->Input();
			return;
		}
	}
}

void State_Game::SaveScores()
{
	std::string score, highScore;
	
	//READ THE FILE
	std::ifstream infile("Resources/Scores.txt");
	if(!infile.is_open())
	{
		assert(false);
	}
	infile >> score >> highScore;
	infile.close();

	//Check if the player has set a new highscore
	score = std::to_string(m_snakes[0]->GetScore());
	if (std::stoi(highScore) < std::stoi(score))
	{
		highScore = score;
	}
	
	std::ofstream outfile("Resources/Scores.txt");
	if(!outfile.is_open())
	{
		assert(false);
	}
	outfile << score << std::endl;
	outfile << highScore << std::endl;
	outfile.close();
}