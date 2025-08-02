#define RAYMATH_STATIC_INLINE // THIS MUST BE THE FIRST THING RELATED TO RAYMATH
#include <raylib.h>
#include <raymath.h>          // raymath.h must be included AFTER raylib.h
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <float.h>            // Required for FLT_MAX

#define MAX_ENTITIES 20 // UPDATED: Renamed from MAX_ENEMIES to reflect all combat entities, increased to 20
#define MAX_CRATES 20
#define BULLET_SPEED 20.0f
#define MAX_PLAYER_BULLETS 80
#define BULLET_MASS 0.2f

#define MAX_ENTITY_BULLETS 40 // Renamed from MAX_ENEMY_BULLETS
#define ENTITY_BULLET_SPEED 15.0f
#define ENTITY_SHOOTING_RANGE 10.0f
#define ENTITY_FIRE_RATE 2.0f

#define MAX_BOMBS 10
#define BOMB_RADIUS 1.0f // Size of the bomb sphere
#define BOMB_FALL_SPEED 20.0f // Initial downward speed
#define BOMB_EXPLOSION_RADIUS 20.0f // Area of effect for bomb damage
#define BOMB_EXPLOSION_DURATION 1.0f // How long explosion effect lasts

// Tank specific defines
#define MAX_TANKS 6 // UPDATED: Number of tanks increased to 6
#define MAX_TANK_BULLETS 20
#define TANK_BULLET_SPEED 25.0f
#define TANK_BULLET_RADIUS 0.2f
#define TANK_FIRE_RATE 1.0f

#define MAX_TANK_BOMBS 5
#define TANK_BOMB_RADIUS 1.5f
#define TANK_BOMB_DROP_RATE 7.0f
#define TANK_BOMB_FALL_SPEED 15.0f
#define TANK_BOMB_EXPLOSION_RADIUS 25.0f
#define TANK_BOMB_EXPLOSION_DURATION 1.5f

// Scaling factor for the tank model
#define TANK_SCALE_FACTOR 3.0f // Make tank 3 times bigger

// Missile specific defines
#define MAX_MISSILES 5 // Max missiles the jet can fire at once
#define MISSILE_SPEED 40.0f // Speed of the missile
#define MISSILE_RADIUS 0.5f // Size of the missile sphere
#define MISSILE_DAMAGE 100.0f // Damage missile deals to tank
#define JET_MISSILE_FIRE_RATE 3.0f // How often jet can fire a missile
#define JET_MISSILE_LOCK_ON_RANGE 70.0f // Distance jet can lock onto a tank

// Define paths for your sound files
#define SOUND_BULLET_PATH "resources/sounds/bullet_shot.wav"
#define SOUND_CRATE_HIT_PATH "resources/sounds/crate_hit.wav"
#define SOUND_ENTITY_SHOT_PATH "resources/sounds/enemy_shot.wav"
#define SOUND_BOMB_DROP_PATH "resources/sounds/bomb_drop.wav"
#define SOUND_EXPLOSION_PATH "resources/sounds/explosion.wav"
#define SOUND_TANK_SHOT_PATH "resources/sounds/tank_shot.wav"
#define SOUND_TANK_BOMB_PATH "resources/sounds/tank_bomb_drop.wav"
#define SOUND_MISSILE_LAUNCH_PATH "resources/sounds/missile_launch.wav" // New sound
#define SOUND_MISSILE_IMPACT_PATH "resources/sounds/missile_impact.wav" // New sound (can reuse explosion)


// Entity Types
typedef enum {
    ENTITY_ENEMY,
    ENTITY_FRIENDLY
} EntityType;

// --- Entity Structs ---
typedef struct {
    Vector3 position;
    bool active;
    Vector3 velocity;
    float mass;
} Bullet;

typedef struct {
    Vector3 position;
    bool active;
    Vector3 velocity;
    float mass;
    float explosionTimer;
    bool exploded;
    float radius; // For different bomb sizes
    float explosion_radius;
    float explosion_duration;
} ProjectileBomb;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    float mass;
    bool active;
    float health;
    float shootTimer;
    EntityType type; // Type of entity (enemy or friendly)
} CombatEntity;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    float mass;
    bool active;
    Color color;
    Quaternion rotation;
    Vector3 angularVelocity;
    bool isPhysicsActive; // Flag to control physics for crates
} Crate;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    float health;
    bool active;
    float bulletShootTimer;
    float bombDropTimer;
    float yawRotation; // For tank orientation
} Vehicle; // To represent the tank

typedef struct {
    Vector3 position;
    bool active;
    Vector3 velocity;
    int targetTankIndex; // Index of the tank it's tracking
    float speed;
    float damage;
} Missile; // New: Missile struct

// --- Global Models and Arrays ---
Model entityModel;
Model crateModel;
Model jetModel;
Model bombModel;
Model tankModel;
Model missileModel; // New: For the missile

Bullet playerBullets[MAX_PLAYER_BULLETS];
Bullet entityBullets[MAX_ENTITY_BULLETS];
Bullet tankBullets[MAX_TANK_BULLETS];
CombatEntity combatEntities[MAX_ENTITIES];
Crate crates[MAX_CRATES];
ProjectileBomb bombs[MAX_BOMBS];
ProjectileBomb tankBombs[MAX_TANK_BOMBS];
Vehicle tanks[MAX_TANKS];
Missile missiles[MAX_MISSILES]; // New: Array of missiles

// --- Global Game Variables ---
Camera camera = { 0 };
float playerHealth = 100.0f;
bool onGround = true;
float jumpVelocity = 0.0f;
float playerBulletTimer = 0.0f;
float playerBulletFireRate = 0.05f;
float walkSpeed = 5.0f;
float runSpeed = 12.5f;
float gravity = 20.0f;
float jumpStrength = 10.0f;
float playerHeight = 2.0f;
float playerRadius = 0.5f;

bool gameOver = false;
int activeEnemiesCount = 0;
int activeFriendliesCount = 0;
int activeTanksCount = 0;

// Jet specific variables
float jetAngle = 0.0f;
float jetRadius = 50.0f; // UPDATED: Jet flies around the whole 100x100 ground
float jetFlightHeight = 30.0f;
float jetSpeed = 0.5f;
float jetDropBombTimer = 0.0f;
float jetBombDropRate = 5.0f;
Vector3 jetCenterPoint = {0.0f, 0.0f, 0.0f}; // Remains centered on the ground
float jetMissileTimer = 0.0f; // New: Timer for jet missile firing
int jetLockedTargetIndex = -1; // New: Index of the tank the jet is currently targeting (-1 if none)

// --- Sounds ---
Sound bulletShotSound;
Sound crateHitSound;
Sound entityShotSound;
Sound bombDropSound;
Sound explosionSound;
Sound tankShotSound;
Sound tankBombSound;
Sound missileLaunchSound;
Sound missileImpactSound;

// --- Custom Collision Functions ---
bool CheckCollisionPointBox3D(Vector3 point, Vector3 boxMin, Vector3 boxMax) {
    return (point.x >= boxMin.x && point.x <= boxMax.x &&
            point.y >= boxMin.y && point.y <= boxMax.y &&
            point.z >= boxMin.z && point.z <= boxMax.z);
}

bool CheckCollisionBoxes3D(Vector3 box1Min, Vector3 box1Max, Vector3 box2Min, Vector3 box2Max) {
    return (box1Min.x <= box2Max.x && box1Max.x >= box2Min.x &&
            box1Min.y <= box2Max.y && box1Max.y >= box2Min.y &&
            box1Min.z <= box2Max.z && box1Max.z >= box2Min.z);
}

// --- Game Initialization/Reset Function ---
void ResetGame() {
    // Reset player
    playerHealth = 100.0f;
    // UPDATED: Player position to one side of the 100x100 ground
    camera.position = (Vector3){ 0.0f, 1.0f, -45.0f };
    camera.target = (Vector3){ 0.0f, 1.0f, -44.0f }; // Look slightly forward
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    jumpVelocity = 0.0f;
    onGround = true;
    gameOver = false;
    DisableCursor();

    // Reset player bullets
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        playerBullets[i].active = false;
    }
    // Reset entity bullets
    for (int i = 0; i < MAX_ENTITY_BULLETS; i++) {
        entityBullets[i].active = false;
    }
    // Reset regular bombs
    for (int i = 0; i < MAX_BOMBS; i++) {
        bombs[i].active = false;
        bombs[i].exploded = false;
        bombs[i].explosionTimer = 0.0f;
    }
    // Reset tank bullets
    for (int i = 0; i < MAX_TANK_BULLETS; i++) {
        tankBullets[i].active = false;
    }
    // Reset tank bombs
    for (int i = 0; i < MAX_TANK_BOMBS; i++) {
        tankBombs[i].active = false;
        tankBombs[i].exploded = false;
        tankBombs[i].explosionTimer = 0.0f;
    }
    // Reset missiles
    for (int i = 0; i < MAX_MISSILES; i++) {
        missiles[i].active = false;
        missiles[i].targetTankIndex = -1; // No target
    }


    // Reset combat entities (enemies and friendly forces)
    activeEnemiesCount = 0;
    activeFriendliesCount = 0;
    for (int i = 0; i < MAX_ENTITIES; i++) { // Loop up to new MAX_ENTITIES
        combatEntities[i].velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
        combatEntities[i].mass = 1.0f;
        combatEntities[i].active = true;
        combatEntities[i].health = 100.0f;
        combatEntities[i].shootTimer = 0.0f;

        // Assign type and spawn position:
        if (rand() % 10 < 6) { // 6 out of 10 chance for enemy
            combatEntities[i].type = ENTITY_ENEMY;
            // UPDATED: Enemies spawn on the positive Z side of the 100x100 ground, spread out
            combatEntities[i].position = (Vector3){ (float)(rand() % 90 - 45), 1.0f, (float)(rand() % 40 + 10) }; // Z from 10 to 49
            activeEnemiesCount++;
        } else {
            combatEntities[i].type = ENTITY_FRIENDLY;
            // UPDATED: Friendlies spawn on the negative Z side of the 100x100 ground, spread out
            combatEntities[i].position = (Vector3){ (float)(rand() % 90 - 45), 1.0f, (float)(rand() % 40 - 50) }; // Z from -50 to -11
            activeFriendliesCount++;
        }
    }

    // Reset crates (5 green, 5 yellow, 10 blue stacked)
    float crateSize = 1.0f;
    float halfCrate = crateSize / 2.0f;

    for (int i = 0; i < 10; i++) {
        crates[i].velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
        crates[i].mass = 2.0f;
        crates[i].active = true;
        crates[i].color = (i < 5) ? GREEN : YELLOW;
        crates[i].rotation = QuaternionIdentity();
        crates[i].angularVelocity = Vector3Zero();
        crates[i].isPhysicsActive = true;

        bool placed = false;
        int attempts = 0;
        while (!placed && attempts < 50) {
            // UPDATED: Random crate positions to cover 100x100 ground
            Vector3 potentialPos = (Vector3){ (float)(rand() % 90 - 45), halfCrate, (float)(rand() % 90 - 45) };
            bool overlap = false;

            for (int j = 0; j < i; j++) {
                if (crates[j].active) {
                    Vector3 box1Min = { potentialPos.x - halfCrate, potentialPos.y - halfCrate, potentialPos.z - halfCrate };
                    Vector3 box1Max = { potentialPos.x + halfCrate, potentialPos.y + halfCrate, potentialPos.z + halfCrate };
                    Vector3 box2Min = { crates[j].position.x - halfCrate, crates[j].position.y - halfCrate, crates[j].position.z - halfCrate };
                    Vector3 box2Max = { crates[j].position.x + halfCrate, crates[j].position.y + halfCrate, crates[j].position.z + halfCrate };

                    if (CheckCollisionBoxes3D(box1Min, box1Max, box2Min, box2Max)) {
                        overlap = true;
                        break;
                    }
                }
            }
            Vector3 playerInitialMin = { camera.position.x - playerRadius, camera.position.y - (playerHeight / 2.0f), camera.position.z - playerRadius };
            Vector3 playerInitialMax = { camera.position.x + playerRadius, camera.position.y + (playerHeight / 2.0f), camera.position.z + playerRadius };
            Vector3 potentialCrateMin = { potentialPos.x - halfCrate, potentialPos.y - halfCrate, potentialPos.z - halfCrate };
            Vector3 potentialCrateMax = { potentialPos.x + halfCrate, potentialPos.y + halfCrate, potentialPos.z + halfCrate };
            if (CheckCollisionBoxes3D(playerInitialMin, playerInitialMax, potentialCrateMin, potentialCrateMax)) {
                overlap = true;
            }

            if (!overlap) {
                crates[i].position = potentialPos;
                placed = true;
            }
            attempts++;
        }
        if (!placed) {
            crates[i].position = (Vector3){ (float)(rand() % 90 - 45), halfCrate, (float)(rand() % 90 - 45) }; // Fallback to new wider random range
            TraceLog(LOG_WARNING, "Failed to place crate %d without overlap after %d attempts. Placed randomly.", i, attempts);
        }
    }
    // New blue stacked crates
    // UPDATED: Stacked crates moved to a corner within the new 100x100 bounds
    Vector3 resetStackBasePosition = { -40.0f, 0.5f, -40.0f };
    for (int i = 0; i < 10; i++) {
        crates[10 + i].position = (Vector3){ resetStackBasePosition.x, resetStackBasePosition.y + i * 1.0f, resetStackBasePosition.z };
        crates[10 + i].velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
        crates[10 + i].mass = 2.0f;
        crates[10 + i].active = true;
        crates[10 + i].color = BLUE;
        crates[10 + i].rotation = QuaternionIdentity();
        crates[10 + i].angularVelocity = Vector3Zero();
        crates[10 + i].isPhysicsActive = false;
    }

    // Initialize the tanks
    activeTanksCount = 0;
    // UPDATED: Tank spawn positions to the positive Z side of the 100x100 ground, for 6 tanks
    Vector3 tankSpawnPositions[MAX_TANKS] = {
        { 40.0f, 1.0f, 40.0f },
        { -40.0f, 1.0f, 40.0f },
        { 0.0f, 1.0f, 45.0f },
        { 30.0f, 1.0f, 35.0f }, // New tank 4
        { -30.0f, 1.0f, 35.0f },// New tank 5
        { 15.0f, 1.0f, 42.0f }  // New tank 6
    };

    for (int i = 0; i < MAX_TANKS; i++) { // Loop up to new MAX_TANKS
        tanks[i].position = tankSpawnPositions[i];
        tanks[i].velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
        tanks[i].health = 200.0f; // Tank has more health
        tanks[i].active = true;
        tanks[i].bulletShootTimer = 0.0f;
        tanks[i].bombDropTimer = 0.0f;
        tanks[i].yawRotation = 0.0f;
        activeTanksCount++;
    }

    // Reset jet missile state
    jetMissileTimer = 0.0f;
    jetLockedTargetIndex = -1;
}

int main(void) {
    // Initialization
    InitWindow(800, 600, "Battle Force");
    SetTargetFPS(60);

    InitAudioDevice();

    // Load sounds
    bulletShotSound = LoadSound(SOUND_BULLET_PATH);
    crateHitSound = LoadSound(SOUND_CRATE_HIT_PATH);
    entityShotSound = LoadSound(SOUND_ENTITY_SHOT_PATH);
    bombDropSound = LoadSound(SOUND_BOMB_DROP_PATH);
    explosionSound = LoadSound(SOUND_EXPLOSION_PATH);
    tankShotSound = LoadSound(SOUND_TANK_SHOT_PATH);
    tankBombSound = LoadSound(SOUND_TANK_BOMB_PATH);
    missileLaunchSound = LoadSound(SOUND_MISSILE_LAUNCH_PATH);
    missileImpactSound = LoadSound(SOUND_MISSILE_IMPACT_PATH);

    SetSoundVolume(bulletShotSound, 0.5f);
    SetSoundVolume(entityShotSound, 0.3f);
    SetSoundVolume(bombDropSound, 0.8f);
    SetSoundVolume(explosionSound, 1.0f);
    SetSoundVolume(tankShotSound, 0.6f);
    SetSoundVolume(tankBombSound, 0.9f);
    SetSoundVolume(missileLaunchSound, 0.7f);
    SetSoundVolume(missileImpactSound, 1.0f);

    // Initialize camera properties
    camera.fovy = 90.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Load models once at initialization
    entityModel = LoadModelFromMesh(GenMeshCube(1.0f, 2.0f, 1.0f));
    crateModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    jetModel = LoadModel("resources/models/Jet.glb");
    bombModel = LoadModelFromMesh(GenMeshSphere(BOMB_RADIUS, 16, 16));
    tankModel = LoadModel("resources/models/Tank.glb");
    missileModel = LoadModelFromMesh(GenMeshCylinder(MISSILE_RADIUS, MISSILE_RADIUS * 3.0f, 16)); // Simple cylinder for missile


    srand(time(NULL));

    ResetGame();

    // Declare these variables ONCE at the top of main to ensure they are always in scope
    float jetYawRotation = 0.0f;

    // Main game loop
    while (!WindowShouldClose()) {

        float deltaTime = GetFrameTime();

        if (!gameOver) {
            // Update camera rotation using Raylib's FPS mode
            UpdateCamera(&camera, CAMERA_FIRST_PERSON);

            // Player movement
            Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
            Vector3 right = Vector3CrossProduct(forward, camera.up);
            right.y = 0;
            right = Vector3Normalize(right);

            Vector3 move = { 0.0f, 0.0f, 0.0f };
            float currentSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? runSpeed : walkSpeed;
            if (IsKeyDown(KEY_W)) move = Vector3Add(move, Vector3Scale(forward, currentSpeed * deltaTime));
            if (IsKeyDown(KEY_S)) move = Vector3Add(move, Vector3Scale(forward, -currentSpeed * deltaTime));
            if (IsKeyDown(KEY_D)) move = Vector3Add(move, Vector3Scale(right, currentSpeed * deltaTime));
            if (IsKeyDown(KEY_A)) move = Vector3Add(move, Vector3Scale(right, -currentSpeed * deltaTime));

            Vector3 prevCameraPosition = camera.position;
            camera.position = Vector3Add(camera.position, move);
            Vector3 lookDirection = Vector3Normalize(Vector3Subtract(camera.target, prevCameraPosition));
            camera.target = Vector3Add(camera.position, lookDirection);

            // Jump mechanics
            if (IsKeyPressed(KEY_SPACE) && onGround) {
                jumpVelocity = jumpStrength;
                onGround = false;
            }

            // Apply gravity and update vertical position
            jumpVelocity -= gravity * deltaTime;
            float newY = camera.position.y + jumpVelocity * deltaTime;
            float playerFeetY = newY - (playerHeight / 2.0f);
            onGround = false;

            // Player-crate vertical collision (standing on top)
            for (int i = 0; i < MAX_CRATES; i++) {
                if (crates[i].active) {
                    float crateTopY = crates[i].position.y + 0.5f;
                    bool horizontalOverlap = (camera.position.x + playerRadius > crates[i].position.x - 0.5f &&
                                              camera.position.x - playerRadius < crates[i].position.x + 0.5f &&
                                              camera.position.z + playerRadius > crates[i].position.z - 0.5f &&
                                              camera.position.z - playerRadius < crates[i].position.z + 0.5f);

                    if (horizontalOverlap && jumpVelocity <= 0 && playerFeetY <= crateTopY && (prevCameraPosition.y - (playerHeight / 2.0f)) >= crateTopY) {
                        newY = crateTopY + (playerHeight / 2.0f);
                        jumpVelocity = 0.0f;
                        onGround = true;
                        // If player lands on a blue crate, activate its physics
                        if (crates[i].color.r == BLUE.r && crates[i].color.g == BLUE.g && crates[i].color.b == BLUE.b) {
                             crates[i].isPhysicsActive = true;
                        }
                        break;
                    }
                }
            }

            // Ground check
            if (!onGround && newY <= 1.0f) {
                newY = 1.0f;
                jumpVelocity = 0.0f;
                onGround = true;
            }
            camera.position.y = newY;
            camera.target = Vector3Add(camera.position, lookDirection);

            Vector3 playerMin = { camera.position.x - playerRadius, camera.position.y - (playerHeight / 2.0f), camera.position.z - playerRadius };
            Vector3 playerMax = { camera.position.x + playerRadius, camera.position.y + (playerHeight / 2.0f), camera.position.z + playerRadius };

            // Update combat entities (chase target and shoot)
            float chaseSpeed = 3.0f;
            for (int i = 0; i < MAX_ENTITIES; i++) {
                if (combatEntities[i].active) {
                    Vector3 targetPosition = Vector3Zero();
                    bool hasTarget = false;

                    // Determine target based on entity type
                    if (combatEntities[i].type == ENTITY_ENEMY) {
                        // Enemies prioritize player
                        if (Vector3Distance(combatEntities[i].position, camera.position) < 25.0f) { // Range for player targeting
                            targetPosition = camera.position;
                            hasTarget = true;
                        } else { // Then look for friendly forces
                            for (int j = 0; j < MAX_ENTITIES; j++) {
                                if (combatEntities[j].active && combatEntities[j].type == ENTITY_FRIENDLY) {
                                    if (Vector3Distance(combatEntities[i].position, combatEntities[j].position) < 25.0f) {
                                        targetPosition = combatEntities[j].position;
                                        hasTarget = true;
                                        break;
                                    }
                                }
                            }
                            // Also target tanks if they are enemies
                            if (!hasTarget) {
                                for (int j = 0; j < MAX_TANKS; j++) {
                                    if (tanks[j].active) { // Tanks are always considered enemies for combat entities
                                        if (Vector3Distance(combatEntities[i].position, tanks[j].position) < 25.0f) {
                                            targetPosition = tanks[j].position;
                                            hasTarget = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    } else { // ENTITY_FRIENDLY
                        // Friendly forces target enemies
                        for (int j = 0; j < MAX_ENTITIES; j++) {
                            if (combatEntities[j].active && combatEntities[j].type == ENTITY_ENEMY) {
                                if (Vector3Distance(combatEntities[i].position, combatEntities[j].position) < 25.0f) {
                                    targetPosition = combatEntities[j].position;
                                    hasTarget = true;
                                    break;
                                }
                            }
                        }
                        // Also target tanks if they are enemies
                        if (!hasTarget) {
                            for (int j = 0; j < MAX_TANKS; j++) {
                                if (tanks[j].active) { // Tanks are always considered enemies for friendly entities
                                    if (Vector3Distance(combatEntities[i].position, tanks[j].position) < 25.0f) {
                                        targetPosition = tanks[j].position;
                                        hasTarget = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    if (hasTarget) {
                        Vector3 directionToTarget = Vector3Normalize(Vector3Subtract(targetPosition, combatEntities[i].position));
                        Vector3 force = Vector3Scale(directionToTarget, chaseSpeed);
                        combatEntities[i].velocity = Vector3Add(combatEntities[i].velocity, Vector3Scale(force, deltaTime / combatEntities[i].mass));

                        // Shooting logic
                        float distanceToTarget = Vector3Distance(combatEntities[i].position, targetPosition);
                        combatEntities[i].shootTimer += deltaTime;

                        if (distanceToTarget <= ENTITY_SHOOTING_RANGE && combatEntities[i].shootTimer >= ENTITY_FIRE_RATE) {
                            for (int k = 0; k < MAX_ENTITY_BULLETS; k++) {
                                if (!entityBullets[k].active) {
                                    entityBullets[k].position = combatEntities[i].position;
                                    // Aim slightly higher for player, or at center for other entities
                                    Vector3 aimTarget = Vector3Equals(targetPosition, camera.position) ? (Vector3){targetPosition.x, targetPosition.y + 0.5f, targetPosition.z} : targetPosition;
                                    Vector3 bulletDirection = Vector3Normalize(Vector3Subtract(aimTarget, combatEntities[i].position));
                                    entityBullets[k].velocity = Vector3Scale(bulletDirection, ENTITY_BULLET_SPEED);
                                    entityBullets[k].active = true;
                                    entityBullets[k].mass = BULLET_MASS;
                                    combatEntities[i].shootTimer = 0.0f;
                                    PlaySound(entityShotSound);
                                    break;
                                }
                            }
                        }
                    } else {
                        // If no target, gradually slow down
                        combatEntities[i].velocity = Vector3Scale(combatEntities[i].velocity, 0.95f);
                    }

                    combatEntities[i].position = Vector3Add(combatEntities[i].position, Vector3Scale(combatEntities[i].velocity, deltaTime));
                    if (combatEntities[i].position.y <= 1.0f) {
                        combatEntities[i].position.y = 1.0f;
                        combatEntities[i].velocity.y = 0.0f;
                    }
                }
            }

            // Update crates
            for (int i = 0; i < MAX_CRATES; i++) {
                if (crates[i].active) {
                    // Only apply physics if isPhysicsActive is true
                    if (crates[i].isPhysicsActive) {
                        crates[i].velocity.y -= gravity * deltaTime;
                        crates[i].angularVelocity = Vector3Scale(crates[i].angularVelocity, 0.95f);

                        float angle = Vector3Length(crates[i].angularVelocity) * deltaTime;
                        Vector3 axis = Vector3Normalize(crates[i].angularVelocity);
                        if (Vector3LengthSqr(crates[i].angularVelocity) > 0.0001f) {
                            Quaternion frameRotation = QuaternionFromAxisAngle(axis, angle);
                            crates[i].rotation = QuaternionMultiply(crates[i].rotation, frameRotation);
                            crates[i].rotation = QuaternionNormalize(crates[i].rotation);
                        }

                        Vector3 predictedPosition = Vector3Add(crates[i].position, Vector3Scale(crates[i].velocity, deltaTime));
                        if (predictedPosition.y - 0.5f <= 0.0f) {
                            if (crates[i].position.y - 0.5f > 0.0f) { // Only bounce if not already on ground
                                crates[i].velocity.y *= -0.5f; // Simple bounce
                                crates[i].angularVelocity = Vector3Scale(crates[i].angularVelocity, 0.5f); // Dampen angular velocity
                            } else {
                                crates[i].velocity.y = 0.0f; // Stop vertical movement
                                crates[i].angularVelocity = Vector3Scale(crates[i].angularVelocity, 0.5f); // Dampen angular velocity
                            }
                            crates[i].position.y = 0.5f; // Snap to ground
                        } else {
                            crates[i].position.y = predictedPosition.y;
                        }
                        crates[i].position.x = predictedPosition.x;
                        crates[i].position.z = predictedPosition.z;
                        crates[i].velocity = Vector3Scale(crates[i].velocity, 0.9f); // Linear damping
                    } else {
                        // If physics is NOT active, ensure it stays completely still
                        crates[i].velocity = Vector3Zero();
                        crates[i].angularVelocity = Vector3Zero();
                        crates[i].rotation = QuaternionIdentity();
                    }
                }
            }

            // CombatEntity-CombatEntity collisions
            for (int i = 0; i < MAX_ENTITIES; i++) {
                if (combatEntities[i].active) {
                    for (int j = i + 1; j < MAX_ENTITIES; j++) {
                        if (combatEntities[j].active) {
                            Vector3 box1Min = { combatEntities[i].position.x - 0.5f, combatEntities[i].position.y - 1.0f, combatEntities[i].position.z - 0.5f };
                            Vector3 box1Max = { combatEntities[i].position.x + 0.5f, combatEntities[i].position.y + 1.0f, combatEntities[i].position.z + 0.5f };
                            Vector3 box2Min = { combatEntities[j].position.x - 0.5f, combatEntities[j].position.y - 1.0f, combatEntities[j].position.z - 0.5f };
                            Vector3 box2Max = { combatEntities[j].position.x + 0.5f, combatEntities[j].position.y + 1.0f, combatEntities[j].position.z + 0.5f };
                            if (CheckCollisionBoxes3D(box1Min, box1Max, box2Min, box2Max)) {
                                combatEntities[i].velocity = Vector3Scale(combatEntities[i].velocity, -0.5f);
                                combatEntities[j].velocity = Vector3Scale(combatEntities[j].velocity, -0.5f);
                            }
                        }
                    }
                }
            }

            // Player-crate horizontal collisions
            for (int i = 0; i < MAX_CRATES; i++) {
                if (crates[i].active) {
                    Vector3 crateMin = { crates[i].position.x - 0.5f, crates[i].position.y - 0.5f, crates[i].position.z - 0.5f };
                    Vector3 crateMax = { crates[i].position.x + 0.5f, crates[i].position.y + 0.5f, crates[i].position.z + 0.5f };
                    if (CheckCollisionBoxes3D(playerMin, playerMax, crateMin, crateMax)) {
                        Vector3 pushDir = Vector3Normalize(move);
                        bool isStandingOnThisCrate = onGround && (fabsf(camera.position.y - (playerHeight / 2.0f) - (crates[i].position.y + 0.5f)) < 0.1f);

                        if (!isStandingOnThisCrate) {
                            crates[i].velocity = Vector3Add(crates[i].velocity, Vector3Scale(pushDir, currentSpeed / crates[i].mass));
                            if (!crates[i].isPhysicsActive) {
                                crates[i].isPhysicsActive = true;
                            }
                        }
                    }
                }
            }

            // Crate-crate collisions (horizontal only)
            for (int i = 0; i < MAX_CRATES; i++) {
                if (crates[i].active) {
                    for (int j = i + 1; j < MAX_CRATES; j++) {
                        if (crates[j].active) {
                            Vector3 box1Min = { crates[i].position.x - 0.5f, crates[i].position.y - 0.5f, crates[i].position.z - 0.5f };
                            Vector3 box1Max = { crates[i].position.x + 0.5f, crates[i].position.y + 0.5f, crates[i].position.z + 0.5f };
                            Vector3 box2Min = { crates[j].position.x - 0.5f, crates[j].position.y - 0.5f, crates[j].position.z - 0.5f };
                            Vector3 box2Max = { crates[j].position.x + 0.5f, crates[j].position.y + 0.5f, crates[j].position.z + 0.5f };
                            if (CheckCollisionBoxes3D(box1Min, box1Max, box2Min, box2Max)) {
                                crates[i].isPhysicsActive = true;
                                crates[j].isPhysicsActive = true;

                                Vector3 collisionNormal = Vector3Normalize(Vector3Subtract(crates[i].position, crates[j].position));
                                if (fabsf(collisionNormal.y) < 0.9f && Vector3LengthSqr(collisionNormal) > 0.001f) {
                                    collisionNormal.y = 0;
                                    collisionNormal = Vector3Normalize(collisionNormal);
                                    crates[i].velocity = Vector3Add(crates[i].velocity, Vector3Scale(collisionNormal, 0.5f));
                                    crates[j].velocity = Vector3Subtract(crates[j].velocity, Vector3Scale(collisionNormal, 0.5f));
                                }

                                if (crates[i].position.y > crates[j].position.y && crates[i].velocity.y < 0) {
                                    float overlap = (crates[i].position.y - 0.5f) - (crates[j].position.y + 0.5f);
                                    if (overlap < 0) {
                                        crates[i].position.y -= overlap;
                                        crates[i].velocity.y *= -0.5f;
                                        crates[i].angularVelocity = Vector3Scale(crates[i].angularVelocity, 0.5f);
                                    }
                                } else if (crates[j].position.y > crates[i].position.y && crates[j].velocity.y < 0) {
                                    float overlap = (crates[j].position.y - 0.5f) - (crates[i].position.y + 0.5f);
                                    if (overlap < 0) {
                                        crates[j].position.y -= overlap;
                                        crates[j].velocity.y *= -0.5f;
                                        crates[j].angularVelocity = Vector3Scale(crates[j].angularVelocity, 0.5f);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Player Shooting
            playerBulletTimer += deltaTime;
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                if (playerBulletTimer >= playerBulletFireRate) {
                    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
                        if (!playerBullets[i].active) {
                            playerBullets[i].position = camera.position;
                            playerBullets[i].velocity = Vector3Scale(Vector3Normalize(Vector3Subtract(camera.target, camera.position)), BULLET_SPEED);
                            playerBullets[i].active = true;
                            playerBullets[i].mass = BULLET_MASS;
                            playerBulletTimer = 0.0f;
                            PlaySound(bulletShotSound);
                            break;
                        }
                    }
                }
            }

            // Update player bullets
            for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
                if (playerBullets[i].active) {
                    playerBullets[i].velocity.y -= gravity * deltaTime;
                    playerBullets[i].position = Vector3Add(playerBullets[i].position, Vector3Scale(playerBullets[i].velocity, deltaTime));
                    if (Vector3Length(playerBullets[i].position) > 100.0f || playerBullets[i].position.y < -5.0f) {
                        playerBullets[i].active = false;
                    }
                }
            }

            // Update entity bullets (from both enemies and friendly forces)
            for (int i = 0; i < MAX_ENTITY_BULLETS; i++) {
                if (entityBullets[i].active) {
                    entityBullets[i].velocity.y -= gravity * deltaTime;
                    entityBullets[i].position = Vector3Add(entityBullets[i].position, Vector3Scale(entityBullets[i].velocity, deltaTime));
                    if (Vector3Length(entityBullets[i].position) > 100.0f || entityBullets[i].position.y < 0.0f) {
                        entityBullets[i].active = false;
                    }
                }
            }

            // Update tank bullets
            for (int i = 0; i < MAX_TANK_BULLETS; i++) {
                if (tankBullets[i].active) {
                    tankBullets[i].velocity.y -= gravity * deltaTime; // Apply gravity to tank bullets
                    tankBullets[i].position = Vector3Add(tankBullets[i].position, Vector3Scale(tankBullets[i].velocity, deltaTime));
                    if (Vector3Length(tankBullets[i].position) > 100.0f || tankBullets[i].position.y < 0.0f) {
                        tankBullets[i].active = false;
                    }
                }
            }

            // Player Bullet-combat entity collisions
            for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
                if (playerBullets[i].active) {
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        if (combatEntities[j].active) {
                            Vector3 boxMin = { combatEntities[j].position.x - 0.5f, combatEntities[j].position.y - 1.0f, combatEntities[j].position.z - 0.5f };
                            Vector3 boxMax = { combatEntities[j].position.x + 0.5f, combatEntities[j].position.y + 1.0f, combatEntities[j].position.z + 0.5f };
                            if (CheckCollisionPointBox3D(playerBullets[i].position, boxMin, boxMax)) {
                                playerBullets[i].active = false;
                                combatEntities[j].health -= 25.0f;
                                if (combatEntities[j].health <= 0) {
                                    combatEntities[j].active = false;
                                    if (combatEntities[j].type == ENTITY_ENEMY) {
                                        activeEnemiesCount--;
                                    } else {
                                        activeFriendliesCount--;
                                    }
                                }
                                break;
                            }
                        }
                    }
                    // Player Bullet-tank collision
                    for (int j = 0; j < MAX_TANKS; j++) {
                        if (tanks[j].active) {
                             // Adjust tank hitbox based on new scale
                             Vector3 tankMin = { tanks[j].position.x - (1.5f * TANK_SCALE_FACTOR), tanks[j].position.y, tanks[j].position.z - (2.5f * TANK_SCALE_FACTOR) };
                             Vector3 tankMax = { tanks[j].position.x + (1.5f * TANK_SCALE_FACTOR), tanks[j].position.y + (1.5f * TANK_SCALE_FACTOR), tanks[j].position.z + (2.5f * TANK_SCALE_FACTOR) };
                            if (CheckCollisionPointBox3D(playerBullets[i].position, tankMin, tankMax)) {
                                playerBullets[i].active = false;
                                tanks[j].health -= 15.0f; // Player bullets do less damage to tank
                                if (tanks[j].health <= 0) {
                                    tanks[j].active = false;
                                    activeTanksCount--;
                                }
                                break; // Bullet hit a tank, stop checking
                            }
                        }
                    }
                }
            }

            // Player Bullet-crate collisions
            for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
                if (playerBullets[i].active) {
                    for (int j = 0; j < MAX_CRATES; j++) {
                        if (crates[j].active) {
                            Vector3 boxMin = { crates[j].position.x - 0.5f, crates[j].position.y - 0.5f, crates[j].position.z - 0.5f };
                            Vector3 boxMax = { crates[j].position.x + 0.5f, crates[j].position.y + 0.5f, crates[j].position.z + 0.5f };
                            if (CheckCollisionPointBox3D(playerBullets[i].position, boxMin, boxMax)) {
                                playerBullets[i].active = false;
                                Vector3 bulletDir = Vector3Normalize(playerBullets[i].velocity);
                                float impulseMagnitude = (playerBullets[i].mass * Vector3Length(playerBullets[i].velocity));
                                crates[j].velocity = Vector3Add(crates[j].velocity, Vector3Scale(bulletDir, impulseMagnitude / crates[j].mass));

                                Vector3 impactPoint = playerBullets[i].position;
                                Vector3 r = Vector3Subtract(impactPoint, crates[j].position);
                                Vector3 forceVector = Vector3Scale(bulletDir, impulseMagnitude);
                                Vector3 torque = Vector3CrossProduct(r, forceVector);

                                float inverseInertia = 1.0f / crates[j].mass;
                                crates[j].angularVelocity = Vector3Add(crates[j].angularVelocity, Vector3Scale(torque, inverseInertia * 0.1f));

                                crates[j].isPhysicsActive = true;

                                float distance = Vector3Distance(camera.position, crates[j].position);
                                float maxDistance = 30.0f;
                                float attenuatedVolume = 1.0f - (distance / maxDistance);
                                if (attenuatedVolume < 0.0f) attenuatedVolume = 0.0f;
                                SetSoundVolume(crateHitSound, attenuatedVolume * 0.7f);

                                Vector3 relativePos = Vector3Subtract(crates[j].position, camera.position);
                                Vector3 cameraRight = Vector3CrossProduct(Vector3Normalize(Vector3Subtract(camera.target, camera.position)), camera.up);
                                float pan = Vector3DotProduct(relativePos, cameraRight) / maxDistance;
                                pan = Clamp(pan, -1.0f, 1.0f);
                                SetSoundPan(crateHitSound, pan);
                                PlaySound(crateHitSound);
                                break;
                            }
                        }
                    }
                }
            }

            // Entity Bullet-player and Entity Bullet-combat entity collisions
            for (int i = 0; i < MAX_ENTITY_BULLETS; i++) {
                if (entityBullets[i].active) {
                    Vector3 bulletMin = { entityBullets[i].position.x - 0.1f, entityBullets[i].position.y - 0.1f, entityBullets[i].position.z - 0.1f };
                    Vector3 bulletMax = { entityBullets[i].position.x + 0.1f, entityBullets[i].position.y + 0.1f, entityBullets[i].position.z + 0.1f };

                    // Collision with player
                    if (CheckCollisionBoxes3D(playerMin, playerMax, bulletMin, bulletMax)) {
                        entityBullets[i].active = false;
                        playerHealth -= 10.0f;
                        if (playerHealth <= 0) {
                            playerHealth = 0;
                            gameOver = true;
                            EnableCursor();
                        }
                        continue; // Bullet hit player, no need to check other entities
                    }

                    // Collision with other combat entities
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        if (combatEntities[j].active) {
                            Vector3 entityBoxMin = { combatEntities[j].position.x - 0.5f, combatEntities[j].position.y - 1.0f, combatEntities[j].position.z - 0.5f };
                            Vector3 entityBoxMax = { combatEntities[j].position.x + 0.5f, combatEntities[j].position.y + 1.0f, combatEntities[j].position.z + 0.5f };
                            if (CheckCollisionBoxes3D(entityBoxMin, entityBoxMax, bulletMin, bulletMax)) {
                                entityBullets[i].active = false;
                                combatEntities[j].health -= 10.0f; // Damage from entity bullets
                                if (combatEntities[j].health <= 0) {
                                    combatEntities[j].active = false;
                                    if (combatEntities[j].type == ENTITY_ENEMY) {
                                        activeEnemiesCount--;
                                    } else {
                                        activeFriendliesCount--;
                                    }
                                }
                                break;
                            }
                        }
                    }
                    // Entity Bullet-tank collision
                    for (int j = 0; j < MAX_TANKS; j++) {
                        if (tanks[j].active) {
                             Vector3 tankMin = { tanks[j].position.x - (1.5f * TANK_SCALE_FACTOR), tanks[j].position.y, tanks[j].position.z - (2.5f * TANK_SCALE_FACTOR) };
                             Vector3 tankMax = { tanks[j].position.x + (1.5f * TANK_SCALE_FACTOR), tanks[j].position.y + (1.5f * TANK_SCALE_FACTOR), tanks[j].position.z + (2.5f * TANK_SCALE_FACTOR) };
                            if (CheckCollisionBoxes3D(tankMin, tankMax, bulletMin, bulletMax)) {
                                entityBullets[i].active = false;
                                tanks[j].health -= 5.0f; // Smaller damage from entity bullets
                                if (tanks[j].health <= 0) {
                                    tanks[j].active = false;
                                    activeTanksCount--;
                                }
                                break;
                            }
                        }
                    }
                }
            }

            // Tank Bullet-player and Tank Bullet-combat entity collisions
            for (int i = 0; i < MAX_TANK_BULLETS; i++) {
                if (tankBullets[i].active) {
                    Vector3 bulletMin = { tankBullets[i].position.x - TANK_BULLET_RADIUS, tankBullets[i].position.y - TANK_BULLET_RADIUS, tankBullets[i].position.z - TANK_BULLET_RADIUS };
                    Vector3 bulletMax = { tankBullets[i].position.x + TANK_BULLET_RADIUS, tankBullets[i].position.y + TANK_BULLET_RADIUS, tankBullets[i].position.z + TANK_BULLET_RADIUS };

                    // Collision with player
                    if (CheckCollisionBoxes3D(playerMin, playerMax, bulletMin, bulletMax)) {
                        tankBullets[i].active = false;
                        playerHealth -= 20.0f; // Tank bullets do more damage
                        if (playerHealth <= 0) {
                            playerHealth = 0;
                            gameOver = true;
                            EnableCursor();
                        }
                        continue;
                    }

                    // Collision with combat entities
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        if (combatEntities[j].active) {
                            Vector3 entityBoxMin = { combatEntities[j].position.x - 0.5f, combatEntities[j].position.y - 1.0f, combatEntities[j].position.z - 0.5f };
                            Vector3 entityBoxMax = { combatEntities[j].position.x + 0.5f, combatEntities[j].position.y + 1.0f, combatEntities[j].position.z + 0.5f };
                            if (CheckCollisionBoxes3D(entityBoxMin, entityBoxMax, bulletMin, bulletMax)) {
                                tankBullets[i].active = false;
                                combatEntities[j].health -= 20.0f; // Tank bullets do more damage to entities
                                if (combatEntities[j].health <= 0) {
                                    combatEntities[j].active = false;
                                    if (combatEntities[j].type == ENTITY_ENEMY) {
                                        activeEnemiesCount--;
                                    } else {
                                        activeFriendliesCount--;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }

            // Player-combat entity collisions (melee damage, only from enemies)
            for (int i = 0; i < MAX_ENTITIES; i++) {
                if (combatEntities[i].active && combatEntities[i].type == ENTITY_ENEMY) { // Only enemies deal melee damage
                    Vector3 entityMin = { combatEntities[i].position.x - 0.5f, combatEntities[i].position.y - 1.0f, combatEntities[i].position.z - 0.5f };
                    Vector3 entityMax = { combatEntities[i].position.x + 0.5f, combatEntities[i].position.y + 1.0f, combatEntities[i].position.z + 0.5f };
                    if (CheckCollisionBoxes3D(playerMin, playerMax, entityMin, entityMax)) {
                        playerHealth -= 10.0f * deltaTime;
                        if (playerHealth <= 0) {
                            playerHealth = 0;
                            gameOver = true;
                            EnableCursor();
                        }
                    }
                }
            }

            // --- Jet and Bomb Logic ---
            // Update jet position and rotation
            jetAngle += jetSpeed * deltaTime;
            if (jetAngle > 2 * PI) jetAngle -= 2 * PI;

            Vector3 currentJetPosition = {
                jetCenterPoint.x + jetRadius * cosf(jetAngle),
                jetFlightHeight,
                jetCenterPoint.z + jetRadius * sinf(jetAngle)
            };

            // Calculate the jet's forward direction (still needed for jet orientation)
            Vector3 nextJetPosition = {
                jetCenterPoint.x + jetRadius * cosf(jetAngle + 0.01f),
                jetFlightHeight,
                jetCenterPoint.z + jetRadius * sinf(jetAngle + 0.01f)
            };
            Vector3 jetForward = Vector3Normalize(Vector3Subtract(nextJetPosition, currentJetPosition));
            jetYawRotation = atan2f(jetForward.x, jetForward.z);

            // Bomb dropping logic: only if there are active enemies
            jetDropBombTimer += deltaTime;
            if (activeEnemiesCount > 0 && jetDropBombTimer >= jetBombDropRate) {
                for (int i = 0; i < MAX_BOMBS; i++) {
                    if (!bombs[i].active) {
                        bombs[i].position = currentJetPosition; // Drop bomb from jet's current position
                        bombs[i].velocity = (Vector3){0.0f, -BOMB_FALL_SPEED, 0.0f};
                        bombs[i].active = true;
                        bombs[i].exploded = false;
                        bombs[i].explosionTimer = 0.0f;
                        bombs[i].radius = BOMB_RADIUS;
                        bombs[i].explosion_radius = BOMB_EXPLOSION_RADIUS;
                        bombs[i].explosion_duration = BOMB_EXPLOSION_DURATION;
                        PlaySound(bombDropSound);
                        jetDropBombTimer = 0.0f; // Reset timer
                        break;
                    }
                }
            }

            // Update regular bombs
            for (int i = 0; i < MAX_BOMBS; i++) {
                if (bombs[i].active) {
                    bombs[i].velocity.y -= gravity * deltaTime;
                    bombs[i].position = Vector3Add(bombs[i].position, Vector3Scale(bombs[i].velocity, deltaTime));

                    if (bombs[i].position.y - bombs[i].radius <= 0.0f && !bombs[i].exploded) {
                        bombs[i].position.y = bombs[i].radius;
                        bombs[i].velocity = Vector3Zero();
                        bombs[i].exploded = true;
                        PlaySound(explosionSound);

                        // Area damage to all combat entities (friendly and enemy)
                        for (int j = 0; j < MAX_ENTITIES; j++) {
                            if (combatEntities[j].active) {
                                float distance = Vector3Distance(bombs[i].position, combatEntities[j].position);
                                if (distance <= bombs[i].explosion_radius) {
                                    combatEntities[j].active = false;
                                    if (combatEntities[j].type == ENTITY_ENEMY) {
                                        activeEnemiesCount--;
                                    } else {
                                        activeFriendliesCount--;
                                    }
                                }
                            }
                        }
                        // Damage tanks
                        for (int j = 0; j < MAX_TANKS; j++) {
                            if (tanks[j].active) {
                                float distance = Vector3Distance(bombs[i].position, tanks[j].position);
                                if (distance <= bombs[i].explosion_radius) {
                                    tanks[j].health -= 50.0f; // Bombs do significant damage to tanks
                                    if (tanks[j].health <= 0) {
                                        tanks[j].active = false;
                                        activeTanksCount--;
                                    }
                                }
                            }
                        }
                        // Also check for player
                        float distToPlayer = Vector3Distance(bombs[i].position, camera.position);
                        if (distToPlayer <= bombs[i].explosion_radius) {
                            playerHealth = 0;
                            gameOver = true;
                            EnableCursor();
                        }
                    }

                    if (bombs[i].exploded) {
                        bombs[i].explosionTimer += deltaTime;
                        if (bombs[i].explosionTimer >= bombs[i].explosion_duration) {
                            bombs[i].active = false;
                        }
                    }
                }
            }
            // --- End Jet and Bomb Logic ---

            // --- Jet Missile Logic ---
            jetMissileTimer += deltaTime;

            // Find closest tank to lock on
            float closestTankDistance = FLT_MAX;
            int potentialTargetIndex = -1;
            for (int i = 0; i < MAX_TANKS; i++) {
                if (tanks[i].active) {
                    float dist = Vector3Distance(currentJetPosition, tanks[i].position);
                    if (dist < closestTankDistance && dist <= JET_MISSILE_LOCK_ON_RANGE) {
                        closestTankDistance = dist;
                        potentialTargetIndex = i;
                    }
                }
            }
            jetLockedTargetIndex = potentialTargetIndex; // Update the jet's locked target

            // Fire missile if target is locked and timer allows
            if (jetLockedTargetIndex != -1 && jetMissileTimer >= JET_MISSILE_FIRE_RATE) {
                for (int i = 0; i < MAX_MISSILES; i++) {
                    if (!missiles[i].active) {
                        missiles[i].position = currentJetPosition; // Missile starts from jet's position
                        missiles[i].velocity = Vector3Scale(jetForward, MISSILE_SPEED); // Initial velocity same as jet's forward
                        missiles[i].active = true;
                        missiles[i].targetTankIndex = jetLockedTargetIndex;
                        missiles[i].speed = MISSILE_SPEED;
                        missiles[i].damage = MISSILE_DAMAGE;
                        PlaySound(missileLaunchSound);
                        jetMissileTimer = 0.0f; // Reset missile fire timer
                        break;
                    }
                }
            }

            // Update missiles
            for (int i = 0; i < MAX_MISSILES; i++) {
                if (missiles[i].active) {
                    // Apply gravity
                    missiles[i].velocity.y -= gravity * deltaTime;

                    // Missile guidance: follow the target tank
                    if (missiles[i].targetTankIndex != -1 && tanks[missiles[i].targetTankIndex].active) {
                        Vector3 targetPos = tanks[missiles[i].targetTankIndex].position;
                        Vector3 directionToTarget = Vector3Normalize(Vector3Subtract(targetPos, missiles[i].position));
                        // Simple proportional navigation: steer towards target
                        missiles[i].velocity = Vector3Lerp(missiles[i].velocity, Vector3Scale(directionToTarget, missiles[i].speed), 2.0f * deltaTime); // Adjust 2.0f for turning speed
                    } else {
                        // If target is destroyed or lost, missile continues straight
                        missiles[i].targetTankIndex = -1; // Clear target
                    }

                    missiles[i].position = Vector3Add(missiles[i].position, Vector3Scale(missiles[i].velocity, deltaTime));

                    // Collision detection with tanks
                    if (missiles[i].targetTankIndex != -1 && tanks[missiles[i].targetTankIndex].active) {
                        Vector3 tankMin = { tanks[missiles[i].targetTankIndex].position.x - (1.5f * TANK_SCALE_FACTOR), tanks[missiles[i].targetTankIndex].position.y, tanks[missiles[i].targetTankIndex].position.z - (2.5f * TANK_SCALE_FACTOR) };
                        Vector3 tankMax = { tanks[missiles[i].targetTankIndex].position.x + (1.5f * TANK_SCALE_FACTOR), tanks[missiles[i].targetTankIndex].position.y + (1.5f * TANK_SCALE_FACTOR), tanks[missiles[i].targetTankIndex].position.z + (2.5f * TANK_SCALE_FACTOR) };

                        if (CheckCollisionPointBox3D(missiles[i].position, tankMin, tankMax)) {
                            tanks[missiles[i].targetTankIndex].health -= missiles[i].damage;
                            if (tanks[missiles[i].targetTankIndex].health <= 0) {
                                tanks[missiles[i].targetTankIndex].active = false;
                                activeTanksCount--;
                            }
                            missiles[i].active = false; // Deactivate missile on impact
                            PlaySound(missileImpactSound);
                        }
                    }

                    // Deactivate missile if it goes too far or hits the ground
                    if (Vector3Length(missiles[i].position) > 150.0f || missiles[i].position.y < 0.0f) {
                        missiles[i].active = false;
                    }
                }
            }
            // --- End Jet Missile Logic ---


            // --- Tank Logic ---
            for (int idx = 0; idx < MAX_TANKS; idx++) {
                if (tanks[idx].active) {
                    // Tank movement and targeting
                    Vector3 tankTargetPosition = Vector3Zero();
                    bool tankHasTarget = false;

                    // Tank prioritizes player
                    if (Vector3Distance(tanks[idx].position, camera.position) < 35.0f * TANK_SCALE_FACTOR) { // Tank has longer target range, scaled
                        tankTargetPosition = camera.position;
                        tankHasTarget = true;
                    } else { // Then look for friendly forces
                        for (int j = 0; j < MAX_ENTITIES; j++) {
                            if (combatEntities[j].active && combatEntities[j].type == ENTITY_FRIENDLY) {
                                if (Vector3Distance(tanks[idx].position, combatEntities[j].position) < 35.0f * TANK_SCALE_FACTOR) {
                                    tankTargetPosition = combatEntities[j].position;
                                    tankHasTarget = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (tankHasTarget) {
                        Vector3 directionToTankTarget = Vector3Normalize(Vector3Subtract(tankTargetPosition, tanks[idx].position));

                        // Update tank rotation to face target
                        tanks[idx].yawRotation = atan2f(directionToTankTarget.x, directionToTankTarget.z);

                        // Move tank towards target
                        float tankMoveSpeed = 2.0f / 3.0f; // Tank movement speed, 1/3 of previous
                        Vector3 tankForce = Vector3Scale(directionToTankTarget, tankMoveSpeed);
                        tanks[idx].velocity = Vector3Add(tanks[idx].velocity, Vector3Scale(tankForce, deltaTime));
                    } else {
                        // Simple patrolling if no target: move randomly
                        if (Vector3LengthSqr(tanks[idx].velocity) < 0.1f) { // If tank stopped
                            tanks[idx].velocity = Vector3Normalize((Vector3){(float)(rand()%20 - 10), 0.0f, (float)(rand()%20 - 10)});
                            tanks[idx].velocity = Vector3Scale(tanks[idx].velocity, 1.0f / 3.0f); // Gentle patrol speed, 1/3 of previous
                        }
                        tanks[idx].velocity = Vector3Scale(tanks[idx].velocity, 0.98f); // Dampen velocity
                        tanks[idx].yawRotation = atan2f(tanks[idx].velocity.x, tanks[idx].velocity.z); // Adjust rotation based on movement
                    }

                    // Apply gravity to tanks
                    tanks[idx].velocity.y -= gravity * deltaTime;
                    tanks[idx].position = Vector3Add(tanks[idx].position, Vector3Scale(tanks[idx].velocity, deltaTime));

                    // Ground collision for tanks
                    if (tanks[idx].position.y < 1.0f) {
                        tanks[idx].position.y = 1.0f;
                        tanks[idx].velocity.y = 0.0f; // Stop vertical movement
                        // Add a slight damping to horizontal velocity when hitting ground
                        tanks[idx].velocity.x *= 0.9f;
                        tanks[idx].velocity.z *= 0.9f;
                    }

                    // Tank-Crate collisions
                    Vector3 tankMin = { tanks[idx].position.x - (1.5f * TANK_SCALE_FACTOR), tanks[idx].position.y, tanks[idx].position.z - (2.5f * TANK_SCALE_FACTOR) };
                    Vector3 tankMax = { tanks[idx].position.x + (1.5f * TANK_SCALE_FACTOR), tanks[idx].position.y + (1.5f * TANK_SCALE_FACTOR), tanks[idx].position.z + (2.5f * TANK_SCALE_FACTOR) };

                    for (int j = 0; j < MAX_CRATES; j++) {
                        if (crates[j].active) {
                            Vector3 crateMin = { crates[j].position.x - 0.5f, crates[j].position.y - 0.5f, crates[j].position.z - 0.5f };
                            Vector3 crateMax = { crates[j].position.x + 0.5f, crates[j].position.y + 0.5f, crates[j].position.z + 0.5f };

                            if (CheckCollisionBoxes3D(tankMin, tankMax, crateMin, crateMax)) {
                                // Simple push effect
                                Vector3 pushDirection = Vector3Normalize(Vector3Subtract(crates[j].position, tanks[idx].position));
                                // Ensure push is primarily horizontal
                                pushDirection.y = 0.0f;
                                pushDirection = Vector3Normalize(pushDirection);

                                float pushStrength = 0.5f; // How hard tank pushes crate
                                crates[j].velocity = Vector3Add(crates[j].velocity, Vector3Scale(pushDirection, pushStrength));
                                crates[j].isPhysicsActive = true; // Activate physics on pushed crate

                                // Also push the tank back slightly to prevent sticking
                                tanks[idx].position = Vector3Subtract(tanks[idx].position, Vector3Scale(pushDirection, 0.1f));
                                tanks[idx].velocity = Vector3Scale(tanks[idx].velocity, 0.5f); // Dampen tank velocity
                            }
                        }
                    }

                    // Tank-CombatEntity collisions
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        if (combatEntities[j].active) {
                            Vector3 entityMin = { combatEntities[j].position.x - 0.5f, combatEntities[j].position.y - 1.0f, combatEntities[j].position.z - 0.5f };
                            Vector3 entityMax = { combatEntities[j].position.x + 0.5f, combatEntities[j].position.y + 1.0f, combatEntities[j].position.z + 0.5f };

                            if (CheckCollisionBoxes3D(tankMin, tankMax, entityMin, entityMax)) {
                                Vector3 pushDirection = Vector3Normalize(Vector3Subtract(combatEntities[j].position, tanks[idx].position));
                                pushDirection.y = 0.0f;
                                pushDirection = Vector3Normalize(pushDirection);

                                float pushStrength = 1.0f; // How hard tank pushes entity
                                combatEntities[j].velocity = Vector3Add(combatEntities[j].velocity, Vector3Scale(pushDirection, pushStrength));

                                // Apply damage to combat entity
                                combatEntities[j].health -= 5.0f * deltaTime; // Continuous damage while colliding
                                if (combatEntities[j].health <= 0) {
                                    combatEntities[j].active = false;
                                    if (combatEntities[j].type == ENTITY_ENEMY) {
                                        activeEnemiesCount--;
                                    } else {
                                        activeFriendliesCount--;
                                    }
                                }
                                // Push tank back slightly too
                                tanks[idx].position = Vector3Subtract(tanks[idx].position, Vector3Scale(pushDirection, 0.05f));
                                tanks[idx].velocity = Vector3Scale(tanks[idx].velocity, 0.8f); // Dampen tank velocity
                            }
                        }
                    }

                    // Tank-Tank collisions (only check with tanks with higher index to avoid double-checking)
                    for (int j = idx + 1; j < MAX_TANKS; j++) {
                        if (tanks[j].active) {
                            Vector3 otherTankMin = { tanks[j].position.x - (1.5f * TANK_SCALE_FACTOR), tanks[j].position.y, tanks[j].position.z - (2.5f * TANK_SCALE_FACTOR) };
                            Vector3 otherTankMax = { tanks[j].position.x + (1.5f * TANK_SCALE_FACTOR), tanks[j].position.y + (1.5f * TANK_SCALE_FACTOR), tanks[j].position.z + (2.5f * TANK_SCALE_FACTOR) };

                            if (CheckCollisionBoxes3D(tankMin, tankMax, otherTankMin, otherTankMax)) {
                                Vector3 collisionAxis = Vector3Normalize(Vector3Subtract(tanks[idx].position, tanks[j].position));
                                collisionAxis.y = 0.0f; // Only resolve horizontal collision
                                collisionAxis = Vector3Normalize(collisionAxis);

                                // Simple repulsion
                                float repulsionStrength = 0.2f;
                                tanks[idx].velocity = Vector3Add(tanks[idx].velocity, Vector3Scale(collisionAxis, repulsionStrength));
                                tanks[j].velocity = Vector3Subtract(tanks[j].velocity, Vector3Scale(collisionAxis, repulsionStrength));

                                // Separate positions slightly to prevent sticking
                                tanks[idx].position = Vector3Add(tanks[idx].position, Vector3Scale(collisionAxis, 0.05f));
                                tanks[j].position = Vector3Subtract(tanks[j].position, Vector3Scale(collisionAxis, 0.05f));
                            }
                        }
                    }

                    // Tank bullet shooting
                    tanks[idx].bulletShootTimer += deltaTime;
                    if (tankHasTarget && Vector3Distance(tanks[idx].position, tankTargetPosition) < 30.0f * TANK_SCALE_FACTOR && tanks[idx].bulletShootTimer >= TANK_FIRE_RATE) {
                        for (int i = 0; i < MAX_TANK_BULLETS; i++) {
                            if (!tankBullets[i].active) {
                                tankBullets[i].position = (Vector3){tanks[idx].position.x, tanks[idx].position.y + (1.0f * TANK_SCALE_FACTOR), tanks[idx].position.z}; // Bullet originates higher, scaled
                                Vector3 aimTarget = Vector3Equals(tankTargetPosition, camera.position) ? (Vector3){tankTargetPosition.x, tankTargetPosition.y + 0.5f, tankTargetPosition.z} : tankTargetPosition;
                                Vector3 bulletDirection = Vector3Normalize(Vector3Subtract(aimTarget, tankBullets[i].position));
                                tankBullets[i].velocity = Vector3Scale(bulletDirection, TANK_BULLET_SPEED);
                                tankBullets[i].active = true;
                                tankBullets[i].mass = BULLET_MASS * 5.0f; // Heavier tank bullets
                                tanks[idx].bulletShootTimer = 0.0f;
                                PlaySound(tankShotSound);
                                break;
                            }
                        }
                    }

                    // Tank bomb dropping
                    tanks[idx].bombDropTimer += deltaTime;
                    if (tankHasTarget && tanks[idx].bombDropTimer >= TANK_BOMB_DROP_RATE) {
                        for (int i = 0; i < MAX_TANK_BOMBS; i++) {
                            if (!tankBombs[i].active) {
                                tankBombs[i].position = (Vector3){tanks[idx].position.x, tanks[idx].position.y + (2.0f * TANK_SCALE_FACTOR), tanks[idx].position.z}; // Drop from above tank, scaled
                                tankBombs[i].velocity = (Vector3){0.0f, -TANK_BOMB_FALL_SPEED, 0.0f};
                                tankBombs[i].active = true;
                                tankBombs[i].exploded = false;
                                tankBombs[i].explosionTimer = 0.0f;
                                tankBombs[i].radius = TANK_BOMB_RADIUS;
                                tankBombs[i].explosion_radius = TANK_BOMB_EXPLOSION_RADIUS;
                                tankBombs[i].explosion_duration = TANK_BOMB_EXPLOSION_DURATION;
                                PlaySound(tankBombSound);
                                tanks[idx].bombDropTimer = 0.0f;
                                break;
                            }
                        }
                    }
                }
            }

            // Update tank bombs
            for (int i = 0; i < MAX_TANK_BOMBS; i++) {
                if (tankBombs[i].active) {
                    tankBombs[i].velocity.y -= gravity * deltaTime;
                    tankBombs[i].position = Vector3Add(tankBombs[i].position, Vector3Scale(tankBombs[i].velocity, deltaTime));

                    if (tankBombs[i].position.y - tankBombs[i].radius <= 0.0f && !tankBombs[i].exploded) {
                        tankBombs[i].position.y = tankBombs[i].radius;
                        tankBombs[i].velocity = Vector3Zero();
                        tankBombs[i].exploded = true;
                        PlaySound(explosionSound); // Use general explosion sound for tank bombs too

                        // Area damage to player
                        float distToPlayer = Vector3Distance(tankBombs[i].position, camera.position);
                        if (distToPlayer <= tankBombs[i].explosion_radius) {
                            playerHealth = 0;
                            gameOver = true;
                            EnableCursor();
                        }

                        // Area damage to combat entities
                        for (int j = 0; j < MAX_ENTITIES; j++) {
                            if (combatEntities[j].active) {
                                float distance = Vector3Distance(tankBombs[i].position, combatEntities[j].position);
                                if (distance <= tankBombs[i].explosion_radius) {
                                    combatEntities[j].active = false;
                                    if (combatEntities[j].type == ENTITY_ENEMY) {
                                        activeEnemiesCount--;
                                    } else {
                                        activeFriendliesCount--;
                                    }
                                }
                            }
                        }
                         // Tank bombs can also damage crates (and potentially other tanks if more were added)
                        for (int j = 0; j < MAX_CRATES; j++) {
                            if (crates[j].active) {
                                float distance = Vector3Distance(tankBombs[i].position, crates[j].position);
                                if (distance <= tankBombs[i].explosion_radius) {
                                    crates[j].active = false; // Destroy crate
                                }
                            }
                        }
                         // Tank bombs can damage other tanks if too close
                        for (int j = 0; j < MAX_TANKS; j++) {
                             if (tanks[j].active) {
                                 float distance = Vector3Distance(tankBombs[i].position, tanks[j].position);
                                 if (distance <= tankBombs[i].explosion_radius) {
                                     tanks[j].health -= 50.0f; // Self-damage for tank
                                     if (tanks[j].health <= 0) {
                                         tanks[j].active = false;
                                         activeTanksCount--;
                                     }
                                 }
                             }
                        }
                    }

                    if (tankBombs[i].exploded) {
                        tankBombs[i].explosionTimer += deltaTime;
                        if (tankBombs[i].explosionTimer >= tankBombs[i].explosion_duration) {
                            tankBombs[i].active = false;
                        }
                    }
                }
            }
            // --- End Tank Logic ---

        } else {
            // Game Over Logic:
            if (IsKeyPressed(KEY_ENTER)) {
                ResetGame();
            }
        }

        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (!gameOver) {
            BeginMode3D(camera);

            // UPDATED: Ground size to 100x100
            DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 100.0f, 100.0f }, GRAY);

            // Draw combat entities (enemies and friendly forces)
            for (int i = 0; i < MAX_ENTITIES; i++) {
                if (combatEntities[i].active) {
                    Color entityColor = (combatEntities[i].type == ENTITY_ENEMY) ? RED : GREEN;
                    DrawCube(combatEntities[i].position, 1.0f, 2.0f, 1.0f, entityColor);
                }
            }

            for (int i = 0; i < MAX_CRATES; i++) {
                if (crates[i].active) {
                    Vector3 rotationAxis;
                    float rotationAngle;
                    QuaternionToAxisAngle(crates[i].rotation, &rotationAxis, &rotationAngle);
                    DrawModelEx(crateModel, crates[i].position, rotationAxis, rotationAngle * RAD2DEG, (Vector3){1.0f, 1.0f, 1.0f}, crates[i].color);
                }
            }

            for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
                if (playerBullets[i].active) {
                    DrawSphere(playerBullets[i].position, 0.1f, DARKBLUE);
                }
            }

            for (int i = 0; i < MAX_ENTITY_BULLETS; i++) {
                if (entityBullets[i].active) {
                    DrawSphere(entityBullets[i].position, 0.1f, ORANGE);
                }
            }

            // Draw tank bullets
            for (int i = 0; i < MAX_TANK_BULLETS; i++) {
                if (tankBullets[i].active) {
                    DrawSphere(tankBullets[i].position, TANK_BULLET_RADIUS, BROWN); // Tank bullets are brown
                }
            }

            // Draw regular bombs (from jet)
            for (int i = 0; i < MAX_BOMBS; i++) {
                if (bombs[i].active && !bombs[i].exploded) {
                    DrawSphere(bombs[i].position, bombs[i].radius, BLACK);
                } else if (bombs[i].active && bombs[i].exploded && bombs[i].explosionTimer < bombs[i].explosion_duration) {
                     DrawSphere(bombs[i].position, bombs[i].explosion_radius * (bombs[i].explosionTimer / bombs[i].explosion_duration), (Color){255, 165, 0, 100});
                }
            }

            // Draw tank bombs
            for (int i = 0; i < MAX_TANK_BOMBS; i++) {
                if (tankBombs[i].active && !tankBombs[i].exploded) {
                    DrawSphere(tankBombs[i].position, tankBombs[i].radius, DARKGRAY); // Tank bombs are dark gray
                } else if (tankBombs[i].active && tankBombs[i].exploded && tankBombs[i].explosionTimer < tankBombs[i].explosion_duration) {
                     DrawSphere(tankBombs[i].position, tankBombs[i].explosion_radius * (tankBombs[i].explosionTimer / tankBombs[i].explosion_duration), (Color){255, 100, 0, 150}); // Slightly different explosion color
                }
            }

            // Draw missiles
            for (int i = 0; i < MAX_MISSILES; i++) {
                if (missiles[i].active) {
                    // Calculate missile orientation to face its velocity direction
                    Vector3 missileForward = Vector3Normalize(missiles[i].velocity);
                    Vector3 missileUp = {0.0f, 1.0f, 0.0f}; // Assume up is always Y-axis for simplicity
                    Vector3 missileRight = Vector3Normalize(Vector3CrossProduct(missileForward, missileUp));
                    missileUp = Vector3Normalize(Vector3CrossProduct(missileRight, missileForward)); // Recalculate up to be orthogonal

                    // Create a transformation matrix for the missile
                    Matrix mat = MatrixIdentity();
                    mat.m0 = missileRight.x; mat.m4 = missileUp.x; mat.m8 = missileForward.x;
                    mat.m1 = missileRight.y; mat.m5 = missileUp.y; mat.m9 = missileForward.y;
                    mat.m2 = missileRight.z; mat.m6 = missileUp.z; mat.m10 = missileForward.z;
                    mat.m12 = missiles[i].position.x; mat.m13 = missiles[i].position.y; mat.m14 = missiles[i].position.z;

                    // Draw the missile as a cylinder (or use a model if you have one)
                    // For now, using DrawModel with a fixed rotation for visual representation.
                    // You might need to adjust the rotation axis/angle for your specific missile model orientation.
                    DrawModel(missileModel, missiles[i].position, 1.0f, RED); // Scale 1.0f, color RED
                }
            }


            // Draw the jet
            Vector3 currentJetPosition = {
                jetCenterPoint.x + jetRadius * cosf(jetAngle),
                jetFlightHeight,
                jetCenterPoint.z + jetRadius * sinf(jetAngle)
            };
            float finalRotationAngle = (jetYawRotation + 0.0f) * RAD2DEG;
            DrawModelEx(jetModel, currentJetPosition, (Vector3){0.0f, 1.0f, 0.0f}, finalRotationAngle, (Vector3){0.1f, 0.1f, 0.1f}, WHITE);

            // Draw the tanks
            for (int i = 0; i < MAX_TANKS; i++) {
                if (tanks[i].active) {
                    DrawModelEx(tankModel, tanks[i].position, (Vector3){0.0f, 1.0f, 0.0f}, tanks[i].yawRotation * RAD2DEG + 180.0f, (Vector3){TANK_SCALE_FACTOR, TANK_SCALE_FACTOR, TANK_SCALE_FACTOR}, WHITE);
                }
            }

            EndMode3D();

            // --- Draw Combat Entity Health Bars (after EndMode3D to draw in 2D overlay) ---
            for (int i = 0; i < MAX_ENTITIES; i++) {
                if (combatEntities[i].active) {
                    Vector3 entityHeadPos = {combatEntities[i].position.x, combatEntities[i].position.y + 1.2f, combatEntities[i].position.z};
                    Vector2 screenPos = GetWorldToScreen(entityHeadPos, camera);

                    int barWidth = 40;
                    int barHeight = 6;
                    int barPadding = 2;

                    float healthPercent = combatEntities[i].health / 100.0f;

                    int outerBarX = (int)screenPos.x - (barWidth / 2) - barPadding;
                    int outerBarY = (int)screenPos.y - (barHeight / 2) - barPadding;
                    int innerBarX = (int)screenPos.x - (barWidth / 2);
                    int innerBarY = (int)screenPos.y - (barHeight / 2);

                    DrawRectangle(outerBarX, outerBarY, barWidth + (barPadding * 2), barHeight + (barPadding * 2), (combatEntities[i].type == ENTITY_ENEMY) ? DARKBROWN : DARKGREEN); // Background for health bar
                    DrawRectangle(innerBarX, innerBarY, (int)(barWidth * healthPercent), barHeight, (combatEntities[i].type == ENTITY_ENEMY) ? RED : GREEN);
                }
            }
            // Draw Tank Health Bars
            for (int i = 0; i < MAX_TANKS; i++) {
                if (tanks[i].active) {
                    Vector3 tankHeadPos = {tanks[i].position.x, tanks[i].position.y + (3.0f * TANK_SCALE_FACTOR), tanks[i].position.z}; // Adjusted height for larger tank
                    Vector2 screenPos = GetWorldToScreen(tankHeadPos, camera);

                    int barWidth = 60;
                    int barHeight = 8;
                    int barPadding = 3;

                    float healthPercent = tanks[i].health / 200.0f; // Max tank health is 200

                    int outerBarX = (int)screenPos.x - (barWidth / 2) - barPadding;
                    int outerBarY = (int)screenPos.y - (barHeight / 2) - barPadding;
                    int innerBarX = (int)screenPos.x - (barWidth / 2);
                    int innerBarY = (int)screenPos.y - (barHeight / 2);

                    DrawRectangle(outerBarX, outerBarY, barWidth + (barPadding * 2), barHeight + (barPadding * 2), DARKBROWN);
                    DrawRectangle(innerBarX, innerBarY, (int)(barWidth * healthPercent), barHeight, MAROON); // Tank health bar color
                }
            }
            // --- End Draw Combat Entity Health Bars ---

            DrawText(TextFormat("Health: %.0f", playerHealth), 10, 10, 20, BLACK);
            DrawText(TextFormat("Enemies: %d", activeEnemiesCount), 10, 40, 20, RED);
            DrawText(TextFormat("Friendlies: %d", activeFriendliesCount), 10, 70, 20, GREEN);
            DrawText(TextFormat("Tanks: %d", activeTanksCount), 10, 100, 20, MAROON); // Display active tanks count
            if (jetLockedTargetIndex != -1 && tanks[jetLockedTargetIndex].active) {
                 DrawText(TextFormat("Jet Target: Tank %d", jetLockedTargetIndex), 10, 130, 20, BLUE);
            } else {
                 DrawText("Jet Target: None", 10, 130, 20, GRAY);
            }


        } else {
            // Game Over Screen drawing
            DrawText("GAME OVER", GetScreenWidth() / 2 - MeasureText("GAME OVER", 40) / 2, GetScreenHeight() / 2 - 20, 40, DARKGRAY);
            DrawText("Press ENTER to Restart", GetScreenWidth() / 2 - MeasureText("Press ENTER to Restart", 20) / 2, GetScreenHeight() / 2 + 30, 20, DARKGRAY);
        }

        EndDrawing();
    }

    // De-Initialization
    UnloadSound(bulletShotSound);
    UnloadSound(crateHitSound);
    UnloadSound(entityShotSound);
    UnloadSound(bombDropSound);
    UnloadSound(explosionSound);
    UnloadSound(tankShotSound);
    UnloadSound(tankBombSound);
    UnloadSound(missileLaunchSound);
    UnloadSound(missileImpactSound);
    UnloadModel(entityModel);
    UnloadModel(crateModel);
    UnloadModel(jetModel);
    UnloadModel(bombModel);
    UnloadModel(tankModel);
    UnloadModel(missileModel); // Unload missile model
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
