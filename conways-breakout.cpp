#include <SFML/Graphics.hpp>
#include <vector>
#include <random>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int CELL_SIZE = 10;
const int GRID_WIDTH = WINDOW_WIDTH / CELL_SIZE;
const int GRID_HEIGHT = (WINDOW_HEIGHT / 2) / CELL_SIZE;

class LifeBreakout {
private:
    std::vector<std::vector<bool>> grid;
    std::vector<std::vector<bool>> nextGrid;
    sf::RectangleShape paddle;
    sf::RenderWindow& window;
    sf::CircleShape ball;
    sf::Vector2f ballVelocity;
    float paddleSpeed = 7.0f;
    sf::Clock lifeClock;
    const float INITIAL_BALL_SPEED = 4.0f;  // Reduced from 7.0f
    const float LIFE_UPDATE_INTERVAL = 1.0f;  // Update life every 1 second
    const float MAX_VERTICAL_ANGLE = 0.85f;  // Prevent too vertical angles
    std::mt19937 rng{std::random_device{}()};  // Random number generator

    void initializeGrid() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 100);

        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                grid[y][x] = (dis(gen) < 15);  // Changed from 20 to 15
            }
        }
        nextGrid = grid;
    }

    int countNeighbors(int x, int y) {
        int count = 0;
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue;
                
                // Use modulo for wrapping around edges
                int newX = (x + i + GRID_WIDTH) % GRID_WIDTH;
                int newY = (y + j + GRID_HEIGHT) % GRID_HEIGHT;
                
                count += grid[newY][newX] ? 1 : 0;
            }
        }
        return count;
    }

    void updateLife() {
        if (lifeClock.getElapsedTime().asSeconds() >= LIFE_UPDATE_INTERVAL) {
            // Create a temporary grid for the next state
            auto tempGrid = grid;  // Make a copy of current state

            // Apply Conway's rules
            for (int y = 0; y < GRID_HEIGHT; y++) {
                for (int x = 0; x < GRID_WIDTH; x++) {
                    int neighbors = countNeighbors(x, y);
                    if (grid[y][x]) {
                        // Live cell
                        tempGrid[y][x] = (neighbors == 2 || neighbors == 3);
                    } else {
                        // Dead cell
                        tempGrid[y][x] = (neighbors == 3);
                    }
                }
            }

            // Update both grids
            grid = tempGrid;
            nextGrid = tempGrid;
            lifeClock.restart();
        }
    }

    void adjustBallVelocity() {
        // Get current velocity angle
        float angle = std::atan2(ballVelocity.y, ballVelocity.x);
        float speed = std::sqrt(ballVelocity.x * ballVelocity.x + ballVelocity.y * ballVelocity.y);

        // If angle is too vertical (close to ±90°), adjust it
        if (std::abs(std::cos(angle)) < 0.3f) {  // Too vertical
            // Add small random adjustment
            std::uniform_real_distribution<float> dist(-0.2f, 0.2f);
            float adjustment = dist(rng);
            
            // Make sure ball moves away from vertical
            if (ballVelocity.x > 0) {
                ballVelocity.x = speed * 0.3f + std::abs(adjustment);
            } else {
                ballVelocity.x = -speed * 0.3f - std::abs(adjustment);
            }
            
            // Adjust y component to maintain speed
            float ySign = ballVelocity.y > 0 ? 1.0f : -1.0f;
            ballVelocity.y = ySign * std::sqrt(speed * speed - ballVelocity.x * ballVelocity.x);
        }
    }

    void checkCollisions() {
        // Ball-Wall collisions
        if (ball.getPosition().x <= 0 || ball.getPosition().x >= WINDOW_WIDTH - ball.getRadius() * 2) {
            ballVelocity.x = -ballVelocity.x;
            adjustBallVelocity();  // Add slight randomization
        }
        if (ball.getPosition().y <= 0) {
            ballVelocity.y = -ballVelocity.y;
            adjustBallVelocity();  // Add slight randomization
        }

        // Ball-Paddle collision
        if (ball.getGlobalBounds().intersects(paddle.getGlobalBounds())) {
            ballVelocity.y = -std::abs(ballVelocity.y);  // Always bounce up
            
            // Add randomization to paddle bounces
            float paddleCenter = paddle.getPosition().x + paddle.getSize().x / 2;
            float ballCenter = ball.getPosition().x + ball.getRadius();
            float difference = ballCenter - paddleCenter;
            
            // Add slight randomization to the bounce
            std::uniform_real_distribution<float> dist(-0.02f, 0.02f);
            float randomFactor = 0.08f + dist(rng);
            
            ballVelocity.x = difference * randomFactor;
            adjustBallVelocity();  // Ensure angle isn't too vertical
        }

        // Ball-Brick collisions
        sf::FloatRect ballBounds = ball.getGlobalBounds();
        
        // Get the grid cells to check (convert ball position to grid coordinates)
        int gridX = static_cast<int>(ball.getPosition().x / CELL_SIZE);
        int gridY = static_cast<int>(ball.getPosition().y / CELL_SIZE);

        // Check surrounding cells for collision
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int checkX = gridX + dx;
                int checkY = gridY + dy;
                
                if (checkX >= 0 && checkX < GRID_WIDTH && 
                    checkY >= 0 && checkY < GRID_HEIGHT && 
                    grid[checkY][checkX]) {
                    
                    sf::FloatRect brickBounds(checkX * CELL_SIZE, checkY * CELL_SIZE, 
                                            CELL_SIZE - 1, CELL_SIZE - 1);
                    
                    if (ballBounds.intersects(brickBounds)) {
                        // Destroy the brick
                        grid[checkY][checkX] = false;
                        nextGrid[checkY][checkX] = false;

                        // Calculate collision point
                        float ballBottom = ballBounds.top + ballBounds.height;
                        float ballRight = ballBounds.left + ballBounds.width;
                        float brickBottom = brickBounds.top + brickBounds.height;
                        float brickRight = brickBounds.left + brickBounds.width;

                        // Find the smallest overlap
                        float overlapLeft = ballRight - brickBounds.left;
                        float overlapRight = brickRight - ballBounds.left;
                        float overlapTop = ballBottom - brickBounds.top;
                        float overlapBottom = brickBottom - ballBounds.top;

                        // Find the smallest overlap
                        float minOverlap = std::min({overlapLeft, overlapRight, overlapTop, overlapBottom});

                        // Bounce based on the collision side
                        if (minOverlap == overlapLeft || minOverlap == overlapRight) {
                            ballVelocity.x = -ballVelocity.x;
                        }
                        if (minOverlap == overlapTop || minOverlap == overlapBottom) {
                            ballVelocity.y = -ballVelocity.y;
                        }

                        // After setting new velocity, adjust if too vertical
                        adjustBallVelocity();

                        return; // Handle one collision per frame
                    }
                }
            }
        }
    }

    void updatePaddle() {
        // Move paddle with arrow keys
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            float newX = paddle.getPosition().x - paddleSpeed;
            paddle.setPosition(std::max(0.f, newX), WINDOW_HEIGHT - 30);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            float newX = paddle.getPosition().x + paddleSpeed;
            paddle.setPosition(std::min(newX, WINDOW_WIDTH - paddle.getSize().x), WINDOW_HEIGHT - 30);
        }
    }

    void resetBall() {
        ball.setPosition(WINDOW_WIDTH / 2 - ball.getRadius(), 
                        WINDOW_HEIGHT / 2 - ball.getRadius());
        
        // Add slight randomization to initial launch
        std::uniform_real_distribution<float> dist(-0.3f, 0.3f);
        float randomAngle = dist(rng);
        ballVelocity = sf::Vector2f(randomAngle * INITIAL_BALL_SPEED, INITIAL_BALL_SPEED);
        
        adjustBallVelocity();  // Ensure initial angle isn't too vertical
    }

    void checkBallLost() {
        // If ball goes below paddle, reset it
        if (ball.getPosition().y > WINDOW_HEIGHT) {
            resetBall();
        }
    }

public:
    LifeBreakout(sf::RenderWindow& win) : window(win) {
        // Initialize grid and nextGrid with proper dimensions
        grid.resize(GRID_HEIGHT, std::vector<bool>(GRID_WIDTH, false));
        nextGrid.resize(GRID_HEIGHT, std::vector<bool>(GRID_WIDTH, false));
        
        // Now call initializeGrid after the vectors are properly sized
        initializeGrid();

        // Initialize paddle
        paddle.setSize(sf::Vector2f(100, 10));
        paddle.setPosition(WINDOW_WIDTH / 2 - 50, WINDOW_HEIGHT - 30);
        paddle.setFillColor(sf::Color::White);

        // Initialize ball
        ball.setRadius(5);
        resetBall();  // Use the new resetBall method
        ball.setFillColor(sf::Color::White);
    }

    void update() {
        updatePaddle();

        // Update ball position
        ball.move(ballVelocity);

        checkCollisions();
        checkBallLost();  // Add this line
        updateLife();
    }

    void draw(sf::RenderWindow& window) {
        // Draw bricks (live cells)
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (grid[y][x]) {
                    sf::RectangleShape cell(sf::Vector2f(CELL_SIZE - 1, CELL_SIZE - 1));
                    cell.setPosition(x * CELL_SIZE, y * CELL_SIZE);
                    
                    // Calculate color based on neighbor count
                    int neighbors = countNeighbors(x, y);
                    
                    // Create different shades of green based on number of neighbors
                    sf::Color cellColor(
                        0,                          // Red
                        150 + (neighbors * 15),     // Green (varies with neighbors)
                        0                           // Blue
                    );
                    
                    cell.setFillColor(cellColor);
                    window.draw(cell);
                }
            }
        }

        window.draw(paddle);
        window.draw(ball);
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Conway's Breakout");
    window.setFramerateLimit(60);

    LifeBreakout game(window);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear(sf::Color::Black);

        game.update();
        game.draw(window);

        window.display();
    }

    return 0;
}
