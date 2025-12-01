#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

// physical constants
float G { 1000.0 };                 // gravitational constant (non-const)
const float time_step { 0.01 };     // time step size
const float radius { 10.0 };        // planetary radius

struct Planet{
    sf::Vector2f pos {};
    sf::Vector2f vel {};
    float mass {};
    sf::CircleShape shape;

    Planet(float x, float y, float m = 10.0) : pos(x, y), vel(0.0, 0.0), mass(m){
        shape.setRadius(radius);
        shape.setOrigin(radius, radius);
        shape.setPosition(pos);
        shape.setFillColor(sf::Color::White);

        if (m >= 100.0){
            shape.setRadius(20.0);
            shape.setFillColor(sf::Color::Red);
        }
    }

    void update(){
        pos += vel * time_step;
        shape.setPosition(pos);
    }
};

float distance(sf::Vector2f a, sf::Vector2f b){
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

sf::Vector2f norm(sf::Vector2f v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    return len > 0 ? v / len : sf::Vector2f(0.0, 0.0);
}

int main(){
    sf::RenderWindow window(sf::VideoMode(1000, 720), "Planet simulator v0");
    window.setFramerateLimit(120); //max 120 fps

    std::vector<Planet> planets;
    bool creatingSmall = false;
    bool creatingBig = false;
    sf::Vector2f createStart;

    while (window.isOpen()){
        sf::Event event;
        while (window.pollEvent(event)){
            // closing the window
            if (event.type == sf::Event::Closed){
                window.close();
            }

            // planet creation
            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    creatingSmall = true;
                    creatingBig = false;
                    createStart = { (float)event.mouseButton.x, (float)event.mouseButton.y };
                }
                if (event.mouseButton.button == sf::Mouse::Right) {
                    creatingSmall = true;
                    creatingBig = true;
                    createStart = { (float)event.mouseButton.x, (float)event.mouseButton.y };
                }
            }

            if (event.type == sf::Event::MouseButtonReleased){
                sf::Vector2f release = { (float)event.mouseButton.x, (float)event.mouseButton.y };
                sf::Vector2f velocity = (createStart - release); // drag opposite direction

                float mass = creatingBig ? 1000.0f : 10.0f;
                Planet p(createStart.x, createStart.y, mass);
                p.vel = velocity;

                planets.push_back(p);
                creatingSmall = false;
            }

            // scrolling increases gravity
            if (event.type == sf::Event::MouseWheelMoved){
                    G *= 2;
                }
        }

        // calculate gravitational attraction
        for (size_t i = 0; i < planets.size(); ++i){
            for (size_t j = i + 1; j < planets.size(); ++j){
                sf::Vector2f dir = planets[j].pos - planets[i].pos;
                float dist = distance(planets[i].pos, planets[j].pos);

                // check collision
                if (dist < 2 * radius) {
                    planets[i].mass = -1;
                    planets[j].mass = -1;
                    continue;
                }

                if (dist > 1e-2){
                    sf::Vector2f force = norm(dir) * (G * planets[i].mass * planets[j].mass) / (dist * dist);
                    planets[i].vel += force / planets[i].mass * time_step;
                    planets[j].vel -= force / planets[j].mass * time_step;
                }
            }
        }
    
        // update planets
        for (auto &p : planets){
            p.update();
        }

        // remove collided planets (negative mass)
        // update this
        planets.erase(std::remove_if(planets.begin(), planets.end(),
            [](const Planet &p){ return p.mass < 0; }), planets.end());

        // plot the planets
        window.clear(sf::Color::Black);
        for (auto &p : planets){
            window.draw(p.shape);
        }

        // title
        sf::Font charter;
        charter.loadFromFile("charterfont.ttf");

        sf::Text text;
        text.setFont(charter);

        text.setString("G is " + std::to_string(int(G)));

        text.setFillColor(sf::Color::Red);
        text.setStyle(sf::Text::Bold | sf::Text::Underlined);
        window.draw(text);

        window.display();
    }

    return 0;
}