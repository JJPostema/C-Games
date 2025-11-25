#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream> // for debugging purposes
#include <cstdlib>
#include <random>
#include <array>

const float INIT_RADIUS { 10.0 }; 
const float DELTA_T { 0.05 };
const float DRAG_SPEED { 0.2 };
const float MAX_SPEED { 25.0 };
const float MASS_DEFICIT { 0.9995 };

// custom colour palette
namespace MyColors {
    // A fixed collection of SFML colors
    static const std::array<sf::Color, 6> palette = {
        sf::Color::Red,
        sf::Color::Green,
        sf::Color::Blue,
        sf::Color::Yellow,
        sf::Color::Cyan,
        sf::Color::Magenta
    };

    // Returns a random color from the palette
    inline sf::Color randomColor() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<std::size_t> dist(0, palette.size() - 1);

        return palette[dist(gen)];
    }
}

// struct that contains food information
// which can be eaten by cell to grow
struct Food {
    sf::Vector2f pos {};
    sf::CircleShape shape;
    bool isAlive;

    Food(float x, float y) : pos(x, y) {
        float r = INIT_RADIUS / 2.0f;

        shape.setRadius(r);
        shape.setOrigin(r, r);
        shape.setPosition(x, y);

        isAlive = true;
        shape.setFillColor(MyColors::randomColor());
    }
};

// base cell struct
struct Cell {
    sf::Vector2f pos {};
    sf::Vector2f vel {};
    float mass {};
    sf::CircleShape shape;
    float effective_radius;

    Cell(float x, float y, float m = 1.0f)
        : pos(x, y), vel(0.0f, 0.0f), mass(m)
    {
        effective_radius = INIT_RADIUS + mass - 1.0f;

        shape.setRadius(effective_radius);
        shape.setOrigin(effective_radius, effective_radius);
        shape.setPosition(x, y);
    }

    void update(){
        pos += vel * DELTA_T;
        effective_radius = INIT_RADIUS + mass - 1.0f;

        shape.setRadius(effective_radius);
        shape.setOrigin(effective_radius, effective_radius);  // update origin after radius change
        shape.setPosition(pos);

        if (mass > 1.0f) {
            mass *= MASS_DEFICIT;
        }
    }

    void kill(){
        mass = -1;
    }
};

// velocity of the cell can not exceed MAX_SPEED
// as the cell gets more massive, MAX_SPEED gets effectively lowered
sf::Vector2f rescaleVelocity(sf::Vector2f v, float mass){
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    return len > MAX_SPEED ? v / len * MAX_SPEED * 50.0f / (50.0f + mass) : v;
}

// calc distance between Cells and Foods
float distance(sf::Vector2f a, sf::Vector2f b){
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}


// -------------------------------- main ------------------------------------------
int main(){
    sf::RenderWindow window(sf::VideoMode(1000, 720), "Agar.io in C++");
    window.setFramerateLimit(120); // max 120 fps

    // initialise vectors that contain information about:
    // Cells: myCell + NPC cells
    // Foods: random blobs that spawn across the map
    // Viruses: TBD, will split any cell
    std::vector<Cell> myCells;
    std::vector<Cell> npcCells;
    std::vector<Food> Foods;
    sf::Vector2f moveTo;

    Cell myCell(0.0, 0.0, 1.0);
    myCells.push_back(myCell);
    bool singleCell = true;

    sf::Font charter;
    charter.loadFromFile("charterfont.ttf");

    sf::Text text;
    text.setFont(charter);

    while (window.isOpen()){

        sf::Event event;
        while (window.pollEvent(event)){
            // closing the window
            if (event.type == sf::Event::Closed){
                window.close();
            }

            // move your cell by clicking
            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    moveTo = { (float)event.mouseButton.x, (float)event.mouseButton.y };
                }

                for (size_t j = 0; j < myCells.size(); ++j){
                    sf::Vector2f velocity = DRAG_SPEED * (moveTo - myCells[j].pos);
                    myCells[j].vel = rescaleVelocity(velocity, myCells[j].mass);
                }
            }

            // split a cell
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    if (singleCell){
                        auto [x, y] = myCells[0].pos;
                        Cell myCell2(x + 1.0f, y + 1.0f, myCells[0].mass / 2);
                        myCells.push_back(myCell2);
                        myCells[0].mass /= 2;
                        myCells[0].vel *= 1.2f;
                        myCells[1].vel = myCells[0].vel / 2.0f;
                        singleCell = false;
                    }
                    else if (!singleCell){
                        myCells[0].mass += myCells[1].mass;
                        myCells[1].mass = -1;
                        singleCell = true;
                    }
                }
            }

        }

        // randomly spawn Foods
        // if eaten, they must disappear: `isAlive` -> false 
        int spawnFood = rand() % 101;
        if (spawnFood < 5){
            int x = rand() % 1081;
            int y = rand() % 720;
            Food f(x, y);
            Foods.push_back(f);
        };

        // food-cell interactions
        for (size_t i = 0; i < Foods.size(); ++i){
            for (size_t j = 0; j < myCells.size(); ++j){
                sf::Vector2f dir = Foods[i].pos - myCells[j].pos;
                float dist = distance(Foods[i].pos, myCells[j].pos);

                // check collision
                if (dist < 3/2 * INIT_RADIUS + myCells[j].mass - 1) {
                    Foods[i].isAlive = false; // food dies
                    myCells[j].mass += 1.0;
                    continue;
                }
            }
        }

        // cell-cell interactions
        // if (myCells.size() > 1){
        //     for (size_t i = 0; i < myCells.size() - 1; ++i){
        //         for (size_t j = i + 1; j < myCells.size(); ++i){
        //             float dist = distance(myCells[i].pos, myCells[j].pos);
        //             if (dist < 0.5f){
        //                 myCells[i].vel *= -0.9f;
        //                 myCells[j].vel *= -0.9f;
        //             }
        //         }
        //     }
        // }

        // update all alive cells
        for (auto &c : myCells){
            c.update();
        }

        // remove eaten food (negative mass)
        // remove dead cells
        Foods.erase(std::remove_if(Foods.begin(), Foods.end(),
            [](const Food &f){ return f.isAlive == false; }), Foods.end());
        myCells.erase(std::remove_if(myCells.begin(), myCells.end(),
            [](const Cell &c){ return c.mass < 0.0f; }), myCells.end());

        // plot the planets
        window.clear(sf::Color::Black);
        for (auto &f : Foods){
            window.draw(f.shape);
        }
        for (auto &c : myCells){
            window.draw(c.shape);
        }
        
        // text
        // text.setString("Cell mass is " + std::to_string(static_cast<int>(myCell.mass)));
        // text.setFillColor(sf::Color::Yellow);
        // text.setStyle(sf::Text::Bold);
        // window.draw(text);
        // sf::FloatRect bounds = text.getLocalBounds();

        // // center the origin
        // text.setOrigin(bounds.left + bounds.width / 2.f,
        //             bounds.top + bounds.height / 2.f);

        // // position at window center
        // text.setPosition(window.getSize().x / 2.f,
        //                 window.getSize().y / 2.f);

        window.display();
    }

    return 0;
}