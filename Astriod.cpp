



#include <SFML/Graphics.hpp>
#include <time.h>
#include <list>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <iostream>

using namespace sf;

const int W = 1200;
const int H = 800;
const float DEGTORAD = 0.017453f;

// ---- Animation ---
class Animation {
public:
    float Frame, speed;
    Sprite sprite;
    std::vector<IntRect> frames;

    Animation() {}
    Animation(Texture& t, int x, int y, int w, int h, int count, float Speed) {
        Frame = 0;
        speed = Speed;
        for (int i = 0; i < count; ++i)
            frames.push_back(IntRect(x + i * w, y, w, h));
        sprite.setTexture(t);
        sprite.setOrigin(w / 2.f, h / 2.f);
        if (!frames.empty()) sprite.setTextureRect(frames[0]);
    }
    void update() {
        Frame += speed;
        int n = (int)frames.size();
        if (n == 0) return;
        if (Frame >= n) Frame -= n;
        sprite.setTextureRect(frames[int(Frame)]);
    }
    bool isEnd() { return Frame + speed >= (int)frames.size(); }
};

// ---- Entity ----
class Entity {
public:
    float x = 0, y = 0, dx = 0, dy = 0, R = 1, angle = 0;
    bool life = true;
    std::string name;
    Animation anim;

    Entity() { life = true; }
    virtual ~Entity() {}
    virtual void update() {}
    void draw(RenderWindow& app) {
        anim.sprite.setPosition(x, y);
        anim.sprite.setRotation(angle + 90);
        app.draw(anim.sprite);
    }
    void settings(const Animation& a, int X, int Y, float Angle = 0, int radius = 1) {
        anim = a;
        x = (float)X; y = (float)Y;
        angle = Angle;
        R = (float)radius;
    }
};

// ---- asteroid -
class asteroid : public Entity {
public:
    asteroid() {
        // use magnitudes similar to what you requested but allow negative direction
        // this yields dx in {-1,0,1} and dy in {-2,-1,0,1,2}
        dx = (float)((rand() % 2) - (rand() % 2));
        dy = (float)((rand() % 3) - (rand() % 3));
        name = "asteroid";
    }
    void update() override {
        x += dx;
        y += dy;
        if (x > W) x = 0; if (x < 0) x = W;
        if (y > H) y = 0; if (y < 0) y = H;
    }
};

// -bullet 
class bullet : public Entity {
public:
    bullet() { name = "bullet"; }
    void update() override {
        dx = cos(angle * DEGTORAD) * 5.f;
        dy = sin(angle * DEGTORAD) * 5.f;
        x += dx; y += dy;
        if (x > W || x < 0 || y > H || y < 0) life = false;
    }
};

// player 
class player : public Entity {
public:
    bool thrust = false;
    player() { name = "player"; thrust = false; }
    void update() override {
        if (thrust) {
            dx += cos(angle * DEGTORAD) * 0.2f;
            dy += sin(angle * DEGTORAD) * 0.2f;
        }
        else {
            dx *= 0.99f; dy *= 0.99f;
        }
        float speed = sqrt(dx * dx + dy * dy);
        if (speed > 15.f) { dx *= 15.f / speed; dy *= 15.f / speed; }
        x += dx; y += dy;
        if (x > W) x = 0; if (x < 0) x = W;
        if (y > H) y = 0; if (y < 0) y = H;
    }
};

//  collision helper 
bool isCollide(Entity* a, Entity* b) {
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float dist2 = dx * dx + dy * dy;
    float r = a->R + b->R;
    return dist2 < r * r;
}

//  utility to delete all entities and free memory 
void clearEntities(std::list<Entity*>& entities) {
    for (Entity* e : entities) delete e;
    entities.clear();
}

//  MAIN 
int main() {
    srand((unsigned)time(nullptr));

    RenderWindow app(VideoMode(W, H), "Asteroids (fixed)");
    app.setFramerateLimit(60);

    // Textures
    Texture t1, t2, t3, t4, t5, t6, t7;
    if (!t1.loadFromFile("images/spaceship.png") ||
        !t2.loadFromFile("images/background.jpg") ||
        !t3.loadFromFile("images/explosions/type_C.png") ||
        !t4.loadFromFile("images/rock.png") ||
        !t5.loadFromFile("images/fire_blue.png") ||
        !t6.loadFromFile("images/rock_small.png") ||
        !t7.loadFromFile("images/explosions/type_B.png")) {
        std::cerr << "Error loading one or more textures. Make sure images/ exists.\n";
        return 1;
    }
    t1.setSmooth(true); t2.setSmooth(true);
    Sprite background(t2);

    // Animations
    Animation sExplosion(t3, 0, 0, 256, 256, 48, 0.5f);
    Animation sRock(t4, 0, 0, 64, 64, 16, 0.2f);
    Animation sRock_small(t6, 0, 0, 64, 64, 16, 0.2f);
    Animation sBullet(t5, 0, 0, 32, 64, 16, 0.8f);
    Animation sPlayer(t1, 40, 0, 40, 40, 1, 0);
    Animation sPlayer_go(t1, 40, 40, 40, 40, 1, 0);
    Animation sExplosion_ship(t7, 0, 0, 192, 192, 64, 0.5f);

    std::list<Entity*> entities;

    // Player (single pointer)
    player* p = new player();
    p->settings(sPlayer, W / 2, H / 2, 0, 20);

    // Score & high score
    int score = 0;
    int highScore = 0;
    {
        std::ifstream in("highscore.txt");
        if (in.good()) in >> highScore;
    }

    Font font;
    if (!font.loadFromFile("font.ttf")) {
        std::cerr << "Error loading font.ttf\n";
        return 1;
    }

    Text scoreText("", font, 30);
    scoreText.setFillColor(Color::White);
    Text highScoreText("", font, 30);
    highScoreText.setFillColor(Color::Yellow);
    highScoreText.setPosition(900.f, 10.f);

    Text startText("PRESS ENTER TO START", font, 50);
    startText.setFillColor(Color::Yellow);
    startText.setPosition(250.f, 350.f);

    Text gameOverText("GAME OVER\n\nENTER = Restart\nESC = Exit", font, 60);
    gameOverText.setFillColor(Color::Red);
    gameOverText.setPosition(260.f, 240.f);

    bool gameStarted = false;
    bool gameOver = false;

    // Spawn initial asteroids
    for (int i = 0; i < 15; ++i) {
        asteroid* a = new asteroid();
        a->settings(sRock, rand() % W, rand() % H, (float)(rand() % 360), 25);
        entities.push_back(a);
    }

    entities.push_back(p);

    // main loop
    while (app.isOpen()) {
        Event event;
        while (app.pollEvent(event)) {
            if (event.type == Event::Closed) app.close();

            // Shoot only when game is running
            if (event.type == Event::KeyPressed &&
                event.key.code == Keyboard::Space &&
                gameStarted && !gameOver)
            {
                bullet* b = new bullet();
                b->settings(sBullet, (int)p->x, (int)p->y, p->angle, 10);
                entities.push_back(b); // pushing bullet is safe here (we're inside event processing, not iterating collision)
            }
        }

        // START screen
        if (!gameStarted) {
            if (Keyboard::isKeyPressed(Keyboard::Enter)) gameStarted = true;
            app.clear();
            app.draw(background);
            app.draw(startText);
            app.display();
            continue;
        }

        // GAME OVER screen
        if (gameOver) {
            app.clear();
            app.draw(background);
            app.draw(gameOverText);
            scoreText.setString("Score: " + std::to_string(score));
            highScoreText.setString("High Score: " + std::to_string(highScore));
            app.draw(scoreText);
            app.draw(highScoreText);
            app.display();

            if (Keyboard::isKeyPressed(Keyboard::Enter)) {
                // Reset all properly
                gameOver = false;
                score = 0;

                // delete existing entities and recreate initial set
                clearEntities(entities);

                p = new player();
                p->settings(sPlayer, W / 2, H / 2, 0, 20);
                entities.push_back(p);

                for (int i = 0; i < 15; ++i) {
                    asteroid* a = new asteroid();
                    a->settings(sRock, rand() % W, rand() % H, (float)(rand() % 360), 25);
                    entities.push_back(a);
                }
            }

            if (Keyboard::isKeyPressed(Keyboard::Escape)) app.close();
            continue;
        }

        // Controls (continuous)
        if (Keyboard::isKeyPressed(Keyboard::Right)) p->angle += 2.f;
        if (Keyboard::isKeyPressed(Keyboard::Left))  p->angle -= 2.f;
        p->thrust = Keyboard::isKeyPressed(Keyboard::Up);

        // --------------------------------------------
        // COLLISION CHECKS (no direct modification of entities)
        // --------------------------------------------
        std::vector<Entity*> toAdd;      // new entities to append after checking collisions
        // mark life = false for dying objects; do not add/erase from entities here
        for (Entity* a : entities) {
            for (Entity* b : entities) {
                if (a == b) continue;

                // bullet hits asteroid
                if (a->name == "asteroid" && b->name == "bullet" && isCollide(a, b)) {
                    a->life = false;
                    b->life = false;

                    // explosion entity
                    Entity* e = new Entity();
                    e->settings(sExplosion, (int)a->x, (int)a->y);
                    e->name = "explosion";
                    toAdd.push_back(e);

                    // give score
                    if (a->R == 25) score += 10;
                    else score += 20;

                    // spawn small asteroids (if large)
                    if (a->R > 15) {
                        for (int k = 0; k < 2; ++k) {
                            asteroid* small = new asteroid();
                            small->settings(sRock_small, (int)a->x, (int)a->y, (float)(rand() % 360), 15);
                            toAdd.push_back(small);
                        }
                    }
                }

                // player hit by asteroid
                if (a->name == "player" && b->name == "asteroid" && isCollide(a, b)) {
                    b->life = false;

                    Entity* e = new Entity();
                    e->settings(sExplosion_ship, (int)a->x, (int)a->y);
                    e->name = "explosion";
                    toAdd.push_back(e);

                    // update high score file
                    if (score > highScore) {
                        highScore = score;
                        std::ofstream out("highscore.txt");
                        if (out.good()) out << highScore;
                    }

                    // end the game
                    gameOver = true;
                }
            }
        }

        // Append pending new entities AFTER the collision loops
        for (Entity* e : toAdd) entities.push_back(e);

        // Update animations/state and remove dead objects safely
        for (auto it = entities.begin(); it != entities.end();) {
            Entity* e = *it;
            e->update();
            e->anim.update();

            if (!e->life) {
                it = entities.erase(it);
                delete e;
            }
            else ++it;
        }

        // occasionally spawn new asteroid
        if (rand() % 150 == 0) {
            asteroid* a = new asteroid();
            a->settings(sRock, 0, rand() % H, (float)(rand() % 360), 25);
            entities.push_back(a);
        }

        // Switch player animation
        p->anim = (p->thrust ? sPlayer_go : sPlayer);

        // Remove finished explosion animations
        for (Entity* e : entities) {
            if (e->name == "explosion" && e->anim.isEnd()) e->life = false;
        }

        // UI
        scoreText.setString("Score: " + std::to_string(score));
        highScoreText.setString("High Score: " + std::to_string(highScore));

        // Draw
        app.clear();
        app.draw(background);
        for (Entity* e : entities) e->draw(app);
        app.draw(scoreText);
        app.draw(highScoreText);
        app.display();
    }

    // cleanup on exit
    clearEntities(entities);
    return 0;
}
