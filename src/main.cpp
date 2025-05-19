#include "raylib.h"
#include <vector>
#include <algorithm>
#include <string>

enum EnemyType { SIMPLE, MID, HARD, BOSS };
enum UpgradeType { HEALTH, FIRE_RATE, ATTACK_RANGE };
enum GameState { PLAYING, GAME_OVER };

class AudioSystem {
private:
    Music backgroundMusic;
    Sound laserSound;
    Sound explosionSound;
    Sound upgradeSound;
    
public:
    AudioSystem() {
        InitAudioDevice();
        backgroundMusic = LoadMusicStream("doom_mus.wav");
        laserSound = LoadSound("lazer-blip.wav");
        explosionSound = LoadSound("explosion.wav");
        upgradeSound = LoadSound("upgrade.wav");
        SetMusicVolume(backgroundMusic, 0.3f);
        SetSoundVolume(laserSound, 0.7f);
        SetSoundVolume(explosionSound, 0.7f);
        SetSoundVolume(upgradeSound, 0.7f);
    }
    
    ~AudioSystem() {
        UnloadMusicStream(backgroundMusic);
        UnloadSound(laserSound);
        UnloadSound(explosionSound);
        UnloadSound(upgradeSound);
        CloseAudioDevice();
    }
    
    void Update() {
        UpdateMusicStream(backgroundMusic);
        if (!IsMusicStreamPlaying(backgroundMusic)) {
            PlayMusicStream(backgroundMusic);
        }
    }
    
    void PlayMusic() {
        PlayMusicStream(backgroundMusic);
    }
    
    void StopMusic() {
        StopMusicStream(backgroundMusic);
    }
    
    void PlayLaser() {
        if (laserSound.frameCount > 0) {
            StopSound(laserSound);
            PlaySound(laserSound);
        }
    }
    
    void PlayExplosion() {
        if (explosionSound.frameCount > 0) {
            PlaySound(explosionSound);
        }
    }
    
    void PlayUpgrade() {
        if (upgradeSound.frameCount > 0) {
            PlaySound(upgradeSound);
        }
    }
};

class GameObject {
protected:
    Vector2 position;
    float speed;
    int health;
    Color color;
    float width, height;

public:
    Rectangle GetHitbox() const { 
        return { position.x, position.y, width, height }; 
    }

    Vector2 GetPosition() const { return position; }
    
    bool IsDead() const { return health <= 0; }
    int GetHealth() const { return health; }
    void TakeDamage(int amount = 1) { health -= amount; }

    virtual bool Update(float dt, std::vector<class Bullet>& bullets, AudioSystem& audio) = 0;
    virtual void Draw() = 0;
    virtual ~GameObject() = default;
};

class Bullet {
public:
    Vector2 position;
    float speed;
    bool isPlayerBullet;
    Color color;
    int damage = 1;

    Bullet(Vector2 pos, bool isPlayer, int dmg = 1) : damage(dmg) {
        position = pos;
        speed = isPlayer ? -500.0f : 300.0f;
        isPlayerBullet = isPlayer;
        color = isPlayer ? YELLOW : RED;
    }

    void Update(float dt) {
        position.y += speed * dt;
    }

    void Draw() {
        DrawCircle(position.x, position.y, 5, color);
    }

    bool IsOutOfScreen() const {
        return position.y < 0 || position.y > GetScreenHeight();
    }
};

class Player : public GameObject {
private:
    float fireCooldown = 0.0f;
    float fireRate = 0.5f;
    float attackRange = 50.0f;
    int maxHealth = 3;

public:
    Player() {
        position = { 400, 550 };
        width = 50;
        height = 20;
        color = BLUE;
        health = maxHealth;
        speed = 300.0f;
    }

    void Reset() {
        position = { 400, 550 };
        health = maxHealth;
        fireCooldown = 0.0f;
    }

    void IncreaseMaxHealth() {
        maxHealth++;
        health++;
    }

    void UpgradeFireRate() {
        fireRate = (fireRate - 0.1f < 0.1f) ? 0.1f : fireRate - 0.1f;
    }

    void UpgradeAttackRange() {
        attackRange += 10.0f;
    }

    bool Update(float dt, std::vector<class Bullet>& bullets, AudioSystem& audio) override {
        if (IsKeyDown(KEY_LEFT)) position.x -= speed * dt;
        if (IsKeyDown(KEY_RIGHT)) position.x += speed * dt;

        if (position.x < 0) position.x = 0;
        if (position.x > GetScreenWidth() - width) position.x = GetScreenWidth() - width;

        fireCooldown -= dt;
        if (fireCooldown <= 0.0f) {
            fireCooldown = fireRate;
            bullets.emplace_back(Vector2{position.x + width/2, position.y}, true);
            audio.PlayLaser();
            return true;
        }
        return false;
    }

    void Draw() override {
        DrawRectangleRec(GetHitbox(), color);
    }
};

class Enemy : public GameObject {
protected:
    float shootTimer = 0.0f;
    float shootInterval = 2.0f;
    
public:
    EnemyType type;
    int scoreValue;

    Enemy(Vector2 pos, EnemyType t) : type(t) {
        position = pos;
        width = (t == BOSS) ? 120.0f : 40.0f;
        height = (t == BOSS) ? 120.0f : 40.0f;
        
        switch(type) {
            case SIMPLE:
                color = RED;
                health = 1;
                speed = 50.0f;
                scoreValue = 10;
                break;
            case MID:
                color = GREEN;
                health = 1;
                speed = 70.0f;
                scoreValue = 25;
                break;
            case HARD:
                color = PURPLE;
                health = 2;
                speed = 90.0f;
                scoreValue = 50;
                break;
            case BOSS:
                color = ORANGE;
                health = 7;
                speed = 40.0f;
                scoreValue = 500;
                break;
        }
    }

    bool Update(float dt, std::vector<Bullet>& bullets, AudioSystem& audio) override {
        if (type == BOSS) {
            static float directionChangeTimer = 0.0f;
            static Vector2 direction = {0, 1};
            
            directionChangeTimer += dt;
            if (directionChangeTimer >= 1.5f) {
                directionChangeTimer = 0.0f;
                direction = {
                    static_cast<float>(GetRandomValue(-10, 10))/10.0f,
                    static_cast<float>(GetRandomValue(5, 10))/10.0f
                };
            }
            
            position.x += direction.x * speed * dt;
            position.y += direction.y * speed * dt;
            
            if (position.x < 0) position.x = 0;
            if (position.x > GetScreenWidth() - width) position.x = GetScreenWidth() - width;
            
            shootTimer += dt;
            if (shootTimer >= 0.5f) {
                shootTimer = 0.0f;
                for (int i = 0; i < 3; i++) {
                    bullets.emplace_back(Vector2{
                        position.x + width/2 + static_cast<float>(GetRandomValue(-20, 20)),
                        position.y + height
                    }, false, 2);
                }
                return true;
            }
        } 
        else {
            position.y += speed * dt;
            
            shootTimer += dt;
            if (shootTimer >= shootInterval) {
                shootTimer = 0.0f;
                bullets.emplace_back(Vector2{position.x + width/2, position.y + height}, false);
                return true;
            }
        }
        return false;
    }

    void Draw() override {
        DrawRectangleRec(GetHitbox(), color);
        if (type == BOSS) {
            DrawRectangle(position.x, position.y - 20, width * (health/7.0f), 10, RED);
        }
    }
};

class Upgrade {
private:
    float fallSpeed = 100.0f;
    
public:
    UpgradeType type;
    Vector2 position;
    float timer = 5.0f;
    bool active = true;

    void Update(float dt) {
        if (!active) return;
        
        position.y += fallSpeed * dt;
        timer -= dt;
    }

    void Apply(Player& player, AudioSystem& audio) {
        if (!active) return;
        
        audio.PlayUpgrade();
        active = false;
    }

    void Draw() {
        if (!active) return;
        
        Color col;
        switch(type) {
            case HEALTH: col = GREEN; break;
            case FIRE_RATE: col = BLUE; break;
            case ATTACK_RANGE: col = YELLOW; break;
        }
        
        DrawRectangle(position.x, position.y, 30, 30, col);
        DrawRectangleLines(position.x, position.y, 30, 30, WHITE);
    }
    
    bool IsActive() const { return active && timer > 0; }
    Rectangle GetHitbox() const { return {position.x, position.y, 30, 30}; }
};

class Game {
private:
    Player player;
    std::vector<Enemy*> enemies;
    std::vector<Bullet> bullets;
    std::vector<Upgrade> upgrades;
    int score = 0;
    float enemySpawnTimer = 0.0f;
    float bossSpawnTimer = 0.0f;
    int bossesDefeated = 0;
    bool bossActive = false;
    AudioSystem audio;
    std::vector<std::pair<std::string, float>> notifications;
    GameState state = PLAYING;

public:
    void Run() {
        InitWindow(800, 600, "Space Invaders (Raylib)");
        SetTargetFPS(60);
        audio.PlayMusic();

        while (!WindowShouldClose()) {
            float dt = GetFrameTime();
            
            if (state == PLAYING) {
                UpdateGame(dt);
            } else if (state == GAME_OVER) {
                if (IsKeyPressed(KEY_ENTER)) {
                    ResetGame();
                    state = PLAYING;
                }
            }
            
            Render();
        }

        Cleanup();
    }

private:
    void ResetGame() {
        // Очистка всех объектов
        for (auto enemy : enemies) delete enemy;
        enemies.clear();
        bullets.clear();
        upgrades.clear();
        notifications.clear();
        
        // Сброс параметров игры
        score = 0;
        enemySpawnTimer = 0.0f;
        bossSpawnTimer = 0.0f;
        bossesDefeated = 0;
        bossActive = false;
        
        // Сброс игрока
        player.Reset();
        
        // Перезапуск музыки
        audio.StopMusic();
        audio.PlayMusic();
    }

    void UpdateGame(float dt) {
        audio.Update();
        player.Update(dt, bullets, audio);
        
        if (player.IsDead()) {
            state = GAME_OVER;
            return;
        }
        
        SpawnEnemies(dt);
        SpawnBoss(dt);
        
        UpdateBullets(dt);
        UpdateEnemies(dt);
        UpdateUpgrades(dt);
        UpdateNotifications(dt);
        
        CheckCollisions();
    }

    void SpawnEnemies(float dt) {
        enemySpawnTimer += dt;
        if (enemySpawnTimer >= 2.0f) {
            enemySpawnTimer = 0.0f;
            int enemyCount = GetRandomValue(1, 4);
            
            for (int i = 0; i < enemyCount; i++) {
                EnemyType type;
                if (bossesDefeated == 0) type = SIMPLE;
                else if (bossesDefeated == 1) type = MID;
                else type = HARD;
                
                Vector2 pos = {
                    static_cast<float>(GetRandomValue(0, GetScreenWidth() - 40)),
                    -40.0f
                };
                
                enemies.push_back(new Enemy(pos, type));
            }
        }
    }

    void SpawnBoss(float dt) {
        if (bossActive) return;
        
        bossSpawnTimer += dt;
        if (bossSpawnTimer >= 60.0f) {
            bossSpawnTimer = 0.0f;
            bossActive = true;
            enemies.push_back(new Enemy(
                Vector2{GetScreenWidth()/2.0f - 60.0f, -120.0f}, 
                BOSS
            ));
            notifications.emplace_back("BOSS INCOMING!", 2.0f);
        }
    }

    void UpdateBullets(float dt) {
        for (auto& bullet : bullets) bullet.Update(dt);
        
        bullets.erase(
            std::remove_if(bullets.begin(), bullets.end(),
                [](const Bullet& b) { return b.IsOutOfScreen(); }),
            bullets.end()
        );
    }

    void UpdateEnemies(float dt) {
        for (auto& enemy : enemies) {
            enemy->Update(dt, bullets, audio);
        }
        
        for (auto it = enemies.begin(); it != enemies.end(); ) {
            if ((*it)->IsDead()) {
                Vector2 dropPos = (*it)->GetPosition();
                
                if ((*it)->type == BOSS) {
                    bossesDefeated++;
                    bossActive = false;
                    SpawnUpgrade(dropPos, true);
                    notifications.emplace_back("BOSS DEFEATED!", 2.0f);
                } else if (GetRandomValue(0, 100) < 20) {
                    SpawnUpgrade(dropPos, false);
                }
                
                score += (*it)->scoreValue;
                delete *it;
                it = enemies.erase(it);
            } else {
                ++it;
            }
        }
    }

    void SpawnUpgrade(Vector2 pos, bool isBossDrop) {
        Upgrade upgrade;
        upgrade.position = pos;
        upgrade.type = static_cast<UpgradeType>(GetRandomValue(0, 2));
        upgrades.push_back(upgrade);
    }

    void UpdateUpgrades(float dt) {
        for (auto& upgrade : upgrades) {
            upgrade.Update(dt);
        }
        
        upgrades.erase(
            std::remove_if(upgrades.begin(), upgrades.end(),
                [](const Upgrade& u) { return !u.IsActive(); }),
            upgrades.end()
        );
    }

    void UpdateNotifications(float dt) {
        for (auto it = notifications.begin(); it != notifications.end(); ) {
            it->second -= dt;
            if (it->second <= 0) {
                it = notifications.erase(it);
            } else {
                ++it;
            }
        }
    }

    void CheckCollisions() {
        // Пули игрока vs Враги
        for (auto bulletIt = bullets.begin(); bulletIt != bullets.end(); ) {
            if (!bulletIt->isPlayerBullet) {
                ++bulletIt;
                continue;
            }

            bool bulletHit = false;
            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end(); ) {
                if (CheckCollisionCircleRec(bulletIt->position, 5, (*enemyIt)->GetHitbox())) {
                    (*enemyIt)->TakeDamage(bulletIt->damage);
                    audio.PlayExplosion();
                    bulletHit = true;
                    break;
                } else {
                    ++enemyIt;
                }
            }

            if (bulletHit) {
                bulletIt = bullets.erase(bulletIt);
            } else {
                ++bulletIt;
            }
        }

        // Враги vs Игрок
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end(); ) {
            if (CheckCollisionRecs(player.GetHitbox(), (*enemyIt)->GetHitbox())) {
                player.TakeDamage((*enemyIt)->type == BOSS ? 2 : 1);
                audio.PlayExplosion();
                ++enemyIt;
            } else {
                ++enemyIt;
            }
        }

        // Пули врагов vs Игрок
        for (auto bulletIt = bullets.begin(); bulletIt != bullets.end(); ) {
            if (bulletIt->isPlayerBullet) {
                ++bulletIt;
                continue;
            }

            if (CheckCollisionCircleRec(bulletIt->position, 5, player.GetHitbox())) {
                player.TakeDamage(bulletIt->damage);
                audio.PlayExplosion();
                bulletIt = bullets.erase(bulletIt);
            } else {
                ++bulletIt;
            }
        }

        // Апгрейды vs Игрок
        for (auto upgradeIt = upgrades.begin(); upgradeIt != upgrades.end(); ) {
            if (upgradeIt->IsActive() && CheckCollisionRecs(player.GetHitbox(), upgradeIt->GetHitbox())) {
                switch(upgradeIt->type) {
                    case HEALTH: 
                        player.IncreaseMaxHealth();
                        notifications.emplace_back("MAX HEALTH +1", 2.0f);
                        break;
                    case FIRE_RATE: 
                        player.UpgradeFireRate();
                        notifications.emplace_back("FIRE RATE UP!", 2.0f);
                        break;
                    case ATTACK_RANGE: 
                        player.UpgradeAttackRange();
                        notifications.emplace_back("ATTACK RANGE +", 2.0f);
                        break;
                }
                upgradeIt->Apply(player, audio);
                upgradeIt = upgrades.erase(upgradeIt);
            } else {
                ++upgradeIt;
            }
        }
    }

    void Render() {
        BeginDrawing();
        ClearBackground(BLACK);

        if (state == PLAYING) {
            // Отрисовка игровых объектов
            for (auto& enemy : enemies) enemy->Draw();
            for (auto& bullet : bullets) bullet.Draw();
            for (auto& upgrade : upgrades) upgrade.Draw();
            player.Draw();

            // Отрисовка интерфейса
            DrawHUD();
            DrawNotifications();
        } else if (state == GAME_OVER) {
            // Экран Game Over
            DrawGameOverScreen();
        }

        EndDrawing();
    }

    void DrawGameOverScreen() {
        // Полупрозрачный черный фон
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 200});
        
        // Надпись Game Over
        const char* gameOverText = "GAME OVER";
        int fontSize = 60;
        Vector2 textSize = MeasureTextEx(GetFontDefault(), gameOverText, fontSize, 2);
        DrawText(gameOverText, 
                GetScreenWidth()/2 - textSize.x/2, 
                GetScreenHeight()/2 - textSize.y - 40, 
                fontSize, RED);
        
        // Счет
        const char* scoreText = TextFormat("YOUR SCORE: %d", score);
        fontSize = 30;
        textSize = MeasureTextEx(GetFontDefault(), scoreText, fontSize, 2);
        DrawText(scoreText, 
                GetScreenWidth()/2 - textSize.x/2, 
                GetScreenHeight()/2, 
                fontSize, WHITE);
        
        // Инструкция для перезапуска
        const char* restartText = "Press ENTER to restart";
        fontSize = 20;
        textSize = MeasureTextEx(GetFontDefault(), restartText, fontSize, 2);
        DrawText(restartText, 
                GetScreenWidth()/2 - textSize.x/2, 
                GetScreenHeight()/2 + 60, 
                fontSize, GREEN);
    }

    void DrawHUD() {
        // Здоровье игрока
        for (int i = 0; i < player.GetHealth(); i++) {
            DrawRectangle(10 + i * 30, 10, 20, 20, RED);
        }
        
        // Счет
        DrawText(TextFormat("Score: %d", score), 10, 40, 20, WHITE);
        
        // Уровень сложности
        const char* difficulty = "";
        if (bossesDefeated == 0) difficulty = "Easy";
        else if (bossesDefeated == 1) difficulty = "Medium";
        else difficulty = "Hard";
        DrawText(TextFormat("Difficulty: %s", difficulty), 10, 70, 20, WHITE);
        
        // Таймер босса
        if (!bossActive) {
            int timeLeft = 60 - static_cast<int>(bossSpawnTimer);
            DrawText(TextFormat("Next boss: %d", timeLeft), 10, 100, 20, WHITE);
        }
    }

    void DrawNotifications() {
        float yPos = 130;
        for (auto& note : notifications) {
            DrawText(note.first.c_str(), 10, yPos, 20, GREEN);
            yPos += 25;
        }
    }

    void Cleanup() {
        for (auto enemy : enemies) delete enemy;
        CloseWindow();
    }
};

int main() {
    Game game;
    game.Run();
    return 0;
}