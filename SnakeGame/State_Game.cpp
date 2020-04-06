#include "State_Game.h"
#include <fstream>
#include "AISnake.h"
#include "PlayerSnake.h"
#include "State_GameOver.h"

/*TODO
 *FIX PATH-FINDING
	*AI SNAKES STOP WRAPPING THEMSELVES UP
	*SOME SORT OF FORWARD-THINKING ALGORITHM
	*A* OR GREEDY BFS SEARCH
 *MAKE UI NICER ON THE EYES
 */


 //BASESTATE METHODS
void State_Game::Initialize(sf::RenderWindow& _window, sf::Font& _font, SoundManager& _soundManager) {
	m_font = _font;

	m_soundManager = _soundManager;

	auto* playerSnake = new PlayerSnake();
	m_snakes.push_back(playerSnake);

	//populate the food array
	for (int i = 0; i < Constants::k_foodAmount; ++i) {
		Food* food = new Food();
		m_foodArray[i] = food;
	}


	//populate the snake Vector
	for (int i = 0; i < Constants::k_AISnakeAmount; ++i) {
		m_snakes.push_back(new AISnake());
	}


	//populate the score UI
	for (unsigned int i = 0; i < m_snakes.size(); ++i) {
		sf::Text playerText;
		playerText.setFont(m_font);
		std::string textToDisplay = "Player";
		textToDisplay += std::to_string(i + 1) + ":";
		playerText.setString(textToDisplay);
		playerText.setFillColor(sf::Color::White);
		playerText.setCharacterSize(25);

		//Work out where they will be positioned
		playerText.setPosition(sf::Vector2f(static_cast<float>(Constants::k_screenWidth - 175), static_cast<float>((i * Constants::k_gameGridCellSize) + 10)));
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

	//Initialise background texture
	m_grassTexture.loadFromFile("Resources/Graphics/Game_Background.png");
	m_grassSprite.setTexture(m_grassTexture);

	//make the snakes know where the food and other snakes are on the screen
	for (auto* snake : m_snakes) {
		for (auto* food : m_foodArray) {
			snake->SetFood(food);
			snake->SetSoundManager(m_soundManager);
		}

		for (auto* otherSnake : m_snakes) {
			if (snake != otherSnake) {
				snake->SetOtherSnake(otherSnake);
			}
		}
	}
}

void State_Game::Update() {
	HandleInput();
	//only play the game if it is paused
	if (!m_paused) {
		m_gobble = false;

		CheckCollisions();

		for (auto* snake : m_snakes) {
			if (snake->GetIsGobbleMode()) {
				m_gobble = true;
			}
			snake->Update();
		}

		//GOBBLE MODE. After a random amount of time, stop Gobble Mode
		if (rand() % 25 == 0 && m_gobble) {
			for (auto* snake : m_snakes) {
				if (snake->GetIsGobbleMode()) {
					snake->SetIsGobbleMode(false);
					m_soundManager.PlaySFX("sfx_gobble_off");
					break;
				}
			}
		}


		UpdateScores();
		//If the player has died, end the game
		auto* playerSnake = dynamic_cast<PlayerSnake*>(m_snakes[0]);
		if (m_snakes[0]->IsDead() && playerSnake) {
			m_soundManager.PlaySFX("sfx_player_snake_death");
			SaveScores();
			current_state = eCurrentState::e_GameOver;
			//Wait until. the player death sound stops...
			core_state.SetState(new State_GameOver());
		}
	}
}

void State_Game::Render(sf::RenderWindow& _window) {

	//Draw the background
	_window.draw(m_grassSprite);

	//Render the food
	for (Food* food : m_foodArray) {
		food->Render(_window);
	}

	//Render the snakes
	for (Snake* snake : m_snakes) {
		snake->Render(_window);
	}

	//Draw the UI
	for (const auto& score : m_scores) {
		_window.draw(score);
	}

	if (m_gobble) {
		_window.draw(*m_gobbleModeText);
	}

	if (m_paused) {
		_window.draw(*m_pausedText);
	}

	//Draw the Walls
	_window.draw(m_topWall.m_wall);
	_window.draw(m_bottomWall.m_wall);
	_window.draw(m_leftWall.m_wall);
	_window.draw(m_rightWall.m_wall);
}

void State_Game::Destroy() {
	for (auto* food : m_foodArray) {
		food = nullptr;
	}

	for (auto* snake : m_snakes) {
		snake = nullptr;
	}
	m_gobbleModeText = nullptr;
	m_pausedText = nullptr;
}

State_Game::~State_Game() {
	for (Food* food : m_foodArray) {
		delete food;
	}

	for (auto* snake : m_snakes) {
		delete snake;
	}
	delete m_gobbleModeText;
	delete m_pausedText;
}


//GAME METHODS
void State_Game::CheckCollisions() {
	for (auto* currentSnake : m_snakes) {
		//only check collisions if the snake is alive
		if (!currentSnake->IsDead()) {
			//Check against Walls
			if (currentSnake->GetHeadPosition().x <= 0 ||
				currentSnake->GetHeadPosition().x > Constants::k_gameWidth + Constants::k_gameGridCellSize ||
				currentSnake->GetHeadPosition().y <= 0 ||
				currentSnake->GetHeadPosition().y > Constants::k_gameHeight + Constants::k_gameGridCellSize) {
				currentSnake->Collision(ECollisionType::e_wall);
				return;
			}
		}
	}
}

void State_Game::RandomiseFood(Food* _foodToRandomise) {
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
				} else {
					_foodToRandomise->Randomise();
				}
			} else {
				_foodToRandomise->Randomise();
			}
		}
	}
}

void State_Game::UpdateScores() {
	for (unsigned int i = 0; i < m_snakes.size(); ++i) {
		std::string textToDisplay = "Player" + std::to_string(i + 1) + ":" + std::to_string(m_snakes[i]->GetScore());
		m_scores[i].setString(textToDisplay);
	}
}

void State_Game::HandleInput() {
	//pause and un-pause the game if escape is pressed
	if (m_escapeKey) {
		m_soundManager.PlaySFX("sfx_menu_pause");
		m_paused = !m_paused;
		m_escapeKey = false;
	}
	//access the player's input function
	for (auto* snake : m_snakes) {
		//There is only ever one player snake
		//If it can be cast to the PlayerSnake type then we have the player
		auto* playerSnake = dynamic_cast<PlayerSnake*>(snake);
		if (playerSnake) {
			if (m_upKey && playerSnake->GetDirection() != EDirection::e_down) {
				playerSnake->SetDirection(EDirection::e_up);
				m_upKey = false;
			}
			if (m_downKey && playerSnake->GetDirection() != EDirection::e_up) {
				playerSnake->SetDirection(EDirection::e_down);
				m_downKey = false;
			}
			if (m_leftKey && playerSnake->GetDirection() != EDirection::e_right) {
				playerSnake->SetDirection(EDirection::e_left);
				m_leftKey = false;
			}
			if (m_rightKey && playerSnake->GetDirection() != EDirection::e_left) {
				playerSnake->SetDirection(EDirection::e_right);
				m_rightKey = false;
			}
			return;
		}
	}
}

void State_Game::SaveScores() {
	std::string score, highScore;

	//READ THE FILE
	std::ifstream infile("Resources/Scores.txt");
	if (!infile.is_open()) {
		assert(false);
	}
	infile >> score >> highScore;
	infile.close();

	//Check if the player has set a new highscore
	score = std::to_string(m_snakes[0]->GetScore());
	if (std::stoi(highScore) < std::stoi(score)) {
		highScore = score;
	}

	std::ofstream outfile("Resources/Scores.txt");
	if (!outfile.is_open()) {
		assert(false);
	}
	outfile << score << std::endl;
	outfile << highScore << std::endl;
	outfile.close();
}