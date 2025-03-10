
#include "visualization/renderer.h"
#include "core/lane.h"
#include "core/vehicle.h"
#include "core/traffic_light.h"
#include "managers/TrafficManager.h"
#include "utils/debuglogger.h"
#include "core/constants.h"

#include <sstream>
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#define M_PI       3.14159265358979323846   // pi


Renderer::Renderer()
    : window(nullptr),
      renderer(nullptr),
      carTexture(nullptr),
      surface(nullptr),
      active(false),
      showDebugOverlay(true),
      frameRateLimit(60),
      lastFrameTime(0),
      windowWidth(800),
      windowHeight(800),
      trafficManager(nullptr) {}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::initialize(int width, int height, const std::string& title) {
    windowWidth = width;
    windowHeight = height;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        DebugLogger::log("Failed to initialize SDL: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Create window
    window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL);
    if (!window) {
        DebugLogger::log("Failed to create window: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        DebugLogger::log("Failed to create renderer: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Load textures
    if (!loadTextures()) {
        DebugLogger::log("Failed to load textures", DebugLogger::LogLevel::ERROR);
        return false;
    }

    active = true;
    DebugLogger::log("Renderer initialized successfully");

    return true;
}

bool Renderer::loadTextures() {
    // Create a simple surface directly with a solid color to avoid SDL_MapRGB issues
    surface = SDL_CreateSurface(20, 10, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        DebugLogger::log("Failed to create surface: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Fill with blue color using a simpler approach
    // Create a color value manually
    Uint32 blueColor = 0x0000FFFF; // RGBA format: blue with full alpha

    // Fill the entire surface with this color
    SDL_FillSurfaceRect(surface, NULL, blueColor);

    carTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    surface = nullptr;

    if (!carTexture) {
        DebugLogger::log("Failed to create car texture: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    return true;
}

void Renderer::startRenderLoop() {
    if (!active || !trafficManager) {
        DebugLogger::log("Cannot start render loop - renderer not active or trafficManager not set", DebugLogger::LogLevel::ERROR);
        return;
    }

    DebugLogger::log("Starting render loop");

    uint32_t lastUpdate = SDL_GetTicks();
    const int updateInterval = 16; // ~60 FPS

    while (active) {
        uint32_t currentTime = SDL_GetTicks();
        uint32_t deltaTime = currentTime - lastUpdate;

        if (deltaTime >= updateInterval) {
            // Process events
            active = processEvents();

            // Update traffic manager
            trafficManager->update(deltaTime);

            // Render frame
            renderFrame();

            lastUpdate = currentTime;
        }

        // Delay to maintain frame rate
        uint32_t frameDuration = SDL_GetTicks() - currentTime;
        if (frameRateLimit > 0) {
            uint32_t targetFrameTime = 1000 / frameRateLimit;
            if (frameDuration < targetFrameTime) {
                SDL_Delay(targetFrameTime - frameDuration);
            }
        }
    }
}

bool Renderer::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                return false;

            case SDL_EVENT_KEY_DOWN: {
                // Check based on the key scancode instead of using SDLK constants
                SDL_Scancode scancode = event.key.scancode;

                // D key scancode is usually 7 (for SDL_SCANCODE_D)
                if (scancode == SDL_SCANCODE_D) {
                    toggleDebugOverlay();
                }
                // Escape key scancode is usually 41 (for SDL_SCANCODE_ESCAPE)
                else if (scancode == SDL_SCANCODE_ESCAPE) {
                    return false;
                }
                break;
            }
        }
    }

    return true;
}

void Renderer::renderFrame() {
    if (!active || !renderer || !trafficManager) {
        return;
    }

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 25, 25, 35, 255); // Dark blue-ish background
    SDL_RenderClear(renderer);

    // Draw roads and lanes
    drawRoadsAndLanes();

    // Draw traffic lights
    drawTrafficLights();

    // Draw vehicles
    drawVehicles();

    // Draw lane labels and direction indicators
    drawLaneLabels();

    // Draw debug overlay if enabled
    if (showDebugOverlay) {
        drawDebugOverlay();
    }

    // Present render
    SDL_RenderPresent(renderer);

    // Update frame time
    lastFrameTime = SDL_GetTicks();
}

void Renderer::drawRoadsAndLanes() {
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;
    const int LANE_WIDTH = Constants::LANE_WIDTH;
    const int CENTER_X = windowWidth / 2;
    const int CENTER_Y = windowHeight / 2;

    // ---------- STEP 1: BACKGROUND ----------
    // Draw dark background for the entire window
    SDL_SetRenderDrawColor(renderer, 25, 25, 35, 255); // Dark blue-ish background
    SDL_RenderClear(renderer);

    // Draw city blocks in corners (buildings)
    drawCityBlocks();

    // ---------- STEP 2: DRAW BASE ROADS ----------
    // Draw horizontal road (dark asphalt)
    SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255); // Darker asphalt
    SDL_FRect horizontalRoad = {
        0, static_cast<float>(CENTER_Y - ROAD_WIDTH/2),
        static_cast<float>(windowWidth), static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &horizontalRoad);

    // Draw vertical road (dark asphalt)
    SDL_FRect verticalRoad = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2), 0,
        static_cast<float>(ROAD_WIDTH), static_cast<float>(windowHeight)
    };
    SDL_RenderFillRect(renderer, &verticalRoad);

    // Draw intersection
    SDL_SetRenderDrawColor(renderer, 35, 35, 40, 255);
    SDL_FRect intersection = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2),
        static_cast<float>(ROAD_WIDTH),
        static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &intersection);

    // Draw road texture (subtle pattern)
    drawRoadTexture();

    // ---------- STEP 3: DRAW LANES WITH GLOWING MARKERS ----------
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Draw lane dividers with glow effect
    drawLaneDividers();

    // Draw traffic lane indicators
    drawLaneIndicators();

    // ---------- STEP 4: DRAW CROSSWALKS ----------
    drawCrosswalks();

    // ---------- STEP 5: DRAW STOP LINES ----------
    drawStopLines();

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Renderer::drawCityBlocks() {
    const int CENTER_X = windowWidth / 2;
    const int CENTER_Y = windowHeight / 2;
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;

    // Buildings color palette (dark with subtle variations)
    SDL_Color buildingColors[] = {
        {45, 45, 60, 255},  // Dark blue-gray
        {50, 50, 65, 255},  // Slightly lighter blue-gray
        {40, 40, 55, 255},  // Darker blue-gray
        {55, 45, 65, 255},  // Purple-ish blue-gray
        {45, 55, 65, 255}   // Teal-ish blue-gray
    };

    std::mt19937 rng(123); // Fixed seed for deterministic "random" buildings
    std::uniform_int_distribution<int> colorDist(0, 4);
    std::uniform_int_distribution<int> heightDist(30, 120);
    std::uniform_int_distribution<int> widthDist(30, 100);
    std::uniform_int_distribution<int> posVariation(5, 20);

    // Top-left quadrant buildings
    for (int x = 20; x < CENTER_X - ROAD_WIDTH/2 - 20; x += widthDist(rng)) {
        for (int y = 20; y < CENTER_Y - ROAD_WIDTH/2 - 20; y += heightDist(rng)) {
            int width = widthDist(rng);
            int height = heightDist(rng);

            // Ensure the building doesn't exceed the quadrant
            if (x + width > CENTER_X - ROAD_WIDTH/2 - 20)
                width = CENTER_X - ROAD_WIDTH/2 - 20 - x;
            if (y + height > CENTER_Y - ROAD_WIDTH/2 - 20)
                height = CENTER_Y - ROAD_WIDTH/2 - 20 - y;

            // Draw building
            SDL_Color color = buildingColors[colorDist(rng)];
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_FRect building = {
                static_cast<float>(x), static_cast<float>(y),
                static_cast<float>(width), static_cast<float>(height)
            };
            SDL_RenderFillRect(renderer, &building);

            // Draw subtle window lights (some lit, some not)
            drawBuildingWindows(x, y, width, height);
        }
    }

    // Similarly for other quadrants with position adjustments
    // Top-right quadrant
    for (int x = CENTER_X + ROAD_WIDTH/2 + 20; x < windowWidth - 20; x += widthDist(rng)) {
        for (int y = 20; y < CENTER_Y - ROAD_WIDTH/2 - 20; y += heightDist(rng)) {
            int width = widthDist(rng);
            int height = heightDist(rng);

            if (x + width > windowWidth - 20)
                width = windowWidth - 20 - x;
            if (y + height > CENTER_Y - ROAD_WIDTH/2 - 20)
                height = CENTER_Y - ROAD_WIDTH/2 - 20 - y;

            SDL_Color color = buildingColors[colorDist(rng)];
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_FRect building = {
                static_cast<float>(x), static_cast<float>(y),
                static_cast<float>(width), static_cast<float>(height)
            };
            SDL_RenderFillRect(renderer, &building);

            drawBuildingWindows(x, y, width, height);
        }
    }

    // Bottom-left quadrant
    for (int x = 20; x < CENTER_X - ROAD_WIDTH/2 - 20; x += widthDist(rng)) {
        for (int y = CENTER_Y + ROAD_WIDTH/2 + 20; y < windowHeight - 20; y += heightDist(rng)) {
            int width = widthDist(rng);
            int height = heightDist(rng);

            if (x + width > CENTER_X - ROAD_WIDTH/2 - 20)
                width = CENTER_X - ROAD_WIDTH/2 - 20 - x;
            if (y + height > windowHeight - 20)
                height = windowHeight - 20 - y;

            SDL_Color color = buildingColors[colorDist(rng)];
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_FRect building = {
                static_cast<float>(x), static_cast<float>(y),
                static_cast<float>(width), static_cast<float>(height)
            };
            SDL_RenderFillRect(renderer, &building);

            drawBuildingWindows(x, y, width, height);
        }
    }

    // Bottom-right quadrant
    for (int x = CENTER_X + ROAD_WIDTH/2 + 20; x < windowWidth - 20; x += widthDist(rng)) {
        for (int y = CENTER_Y + ROAD_WIDTH/2 + 20; y < windowHeight - 20; y += heightDist(rng)) {
            int width = widthDist(rng);
            int height = heightDist(rng);

            if (x + width > windowWidth - 20)
                width = windowWidth - 20 - x;
            if (y + height > windowHeight - 20)
                height = windowHeight - 20 - y;

            SDL_Color color = buildingColors[colorDist(rng)];
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_FRect building = {
                static_cast<float>(x), static_cast<float>(y),
                static_cast<float>(width), static_cast<float>(height)
            };
            SDL_RenderFillRect(renderer, &building);

            drawBuildingWindows(x, y, width, height);
        }
    }
}

void Renderer::drawBuildingWindows(int buildingX, int buildingY, int buildingWidth, int buildingHeight) {
    std::mt19937 rng(buildingX * buildingY); // Seed based on position for deterministic randomness
    std::uniform_int_distribution<int> lightDist(0, 10); // Probability of lit windows

    const int windowSize = 4;
    const int windowMargin = 8;

    for (int x = buildingX + windowMargin; x < buildingX + buildingWidth - windowMargin; x += windowMargin) {
        for (int y = buildingY + windowMargin; y < buildingY + buildingHeight - windowMargin; y += windowMargin) {
            if (lightDist(rng) < 3) { // 30% chance of lit window
                // Lit window (yellow-ish glow)
                SDL_SetRenderDrawColor(renderer, 255, 240, 150, 200);
            } else {
                // Dark window
                SDL_SetRenderDrawColor(renderer, 60, 60, 75, 150);
            }

            SDL_FRect window = {
                static_cast<float>(x), static_cast<float>(y),
                static_cast<float>(windowSize), static_cast<float>(windowSize)
            };
            SDL_RenderFillRect(renderer, &window);
        }
    }
}

void Renderer::drawRoadTexture() {
    const int CENTER_X = windowWidth / 2;
    const int CENTER_Y = windowHeight / 2;
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;

    // Draw subtle dark pattern for road texture
    SDL_SetRenderDrawColor(renderer, 35, 35, 40, 30); // Very subtle dark pattern

    // Horizontal road texture
    for (int x = 0; x < windowWidth; x += 10) {
        for (int y = CENTER_Y - ROAD_WIDTH/2; y < CENTER_Y + ROAD_WIDTH/2; y += 10) {
            if ((x + y) % 20 == 0) {
                SDL_FRect speck = {
                    static_cast<float>(x), static_cast<float>(y),
                    2.0f, 2.0f
                };
                SDL_RenderFillRect(renderer, &speck);
            }
        }
    }

    // Vertical road texture
    for (int x = CENTER_X - ROAD_WIDTH/2; x < CENTER_X + ROAD_WIDTH/2; x += 10) {
        for (int y = 0; y < windowHeight; y += 10) {
            if ((x + y) % 20 == 0) {
                SDL_FRect speck = {
                    static_cast<float>(x), static_cast<float>(y),
                    2.0f, 2.0f
                };
                SDL_RenderFillRect(renderer, &speck);
            }
        }
    }
}

void Renderer::drawLaneDividers() {
    const int CENTER_X = windowWidth / 2;
    const int CENTER_Y = windowHeight / 2;
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;
    const int LANE_WIDTH = Constants::LANE_WIDTH;

    // Horizontal lane dividers
    for (int i = 1; i < 3; i++) {
        float y = CENTER_Y - ROAD_WIDTH/2 + i * LANE_WIDTH;

        if (i == 1) {
            // Center yellow double line with glow effect
            // First, draw a subtle glow
            SDL_SetRenderDrawColor(renderer, 255, 220, 100, 30); // Yellow glow
            SDL_FRect yellowGlow = {
                0, y - 4.0f,
                static_cast<float>(windowWidth), 8.0f
            };
            SDL_RenderFillRect(renderer, &yellowGlow);

            // Then draw the actual double yellow line
            SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255); // Bright yellow
            SDL_FRect yellowLine1 = {
                0, y - 2.0f,
                static_cast<float>(windowWidth), 1.5f
            };
            SDL_FRect yellowLine2 = {
                0, y + 0.5f,
                static_cast<float>(windowWidth), 1.5f
            };
            SDL_RenderFillRect(renderer, &yellowLine1);
            SDL_RenderFillRect(renderer, &yellowLine2);
        } else {
            // White dashed lines with subtle glow
            for (int x = 0; x < windowWidth; x += 40) {
                // Skip intersection area
                if (x >= CENTER_X - ROAD_WIDTH/2 - 10 && x <= CENTER_X + ROAD_WIDTH/2 + 10)
                    continue;

                // Glow
                SDL_SetRenderDrawColor(renderer, 220, 220, 255, 30); // White-blue glow
                SDL_FRect whiteGlow = {
                    static_cast<float>(x), y - 2.0f,
                    25.0f, 4.0f
                };
                SDL_RenderFillRect(renderer, &whiteGlow);

                // Actual line
                SDL_SetRenderDrawColor(renderer, 220, 220, 255, 255); // Bright white-blue
                SDL_FRect whiteLine = {
                    static_cast<float>(x), y - 0.75f,
                    25.0f, 1.5f
                };
                SDL_RenderFillRect(renderer, &whiteLine);
            }
        }
    }

    // Vertical lane dividers
    for (int i = 1; i < 3; i++) {
        float x = CENTER_X - ROAD_WIDTH/2 + i * LANE_WIDTH;

        if (i == 1) {
            // Center yellow double line with glow
            SDL_SetRenderDrawColor(renderer, 255, 220, 100, 30); // Yellow glow
            SDL_FRect yellowGlow = {
                x - 4.0f, 0,
                8.0f, static_cast<float>(windowHeight)
            };
            SDL_RenderFillRect(renderer, &yellowGlow);

            // Actual yellow lines
            SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255); // Bright yellow
            SDL_FRect yellowLine1 = {
                x - 2.0f, 0,
                1.5f, static_cast<float>(windowHeight)
            };
            SDL_FRect yellowLine2 = {
                x + 0.5f, 0,
                1.5f, static_cast<float>(windowHeight)
            };
            SDL_RenderFillRect(renderer, &yellowLine1);
            SDL_RenderFillRect(renderer, &yellowLine2);
        } else {
            // White dashed lines with subtle glow
            for (int y = 0; y < windowHeight; y += 40) {
                // Skip intersection area
                if (y >= CENTER_Y - ROAD_WIDTH/2 - 10 && y <= CENTER_Y + ROAD_WIDTH/2 + 10)
                    continue;

                // Glow
                SDL_SetRenderDrawColor(renderer, 220, 220, 255, 30); // White-blue glow
                SDL_FRect whiteGlow = {
                    x - 2.0f, static_cast<float>(y),
                    4.0f, 25.0f
                };
                SDL_RenderFillRect(renderer, &whiteGlow);

                // Actual line
                SDL_SetRenderDrawColor(renderer, 220, 220, 255, 255); // Bright white-blue
                SDL_FRect whiteLine = {
                    x - 0.75f, static_cast<float>(y),
                    1.5f, 25.0f
                };
                SDL_RenderFillRect(renderer, &whiteLine);
            }
        }
    }
}

void Renderer::drawLaneIndicators() {
    const int CENTER_X = windowWidth / 2;
    const int CENTER_Y = windowHeight / 2;
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;
    const int LANE_WIDTH = Constants::LANE_WIDTH;

    // Lane markers for each lane type

    // --- A1 Lane Indicator (North, Incoming) ---
    drawLaneMarker(CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH/2,
                  CENTER_Y - ROAD_WIDTH/2 - 30,
                  "A1", {100, 150, 255, 200}, true);

    // --- A2 Lane Indicator (North, Priority) ---
    drawLaneMarker(CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH + LANE_WIDTH/2,
                  CENTER_Y - ROAD_WIDTH/2 - 30,
                  "A2", {255, 140, 0, 200}, true);

    // --- A3 Lane Indicator (North, Free) ---
    drawLaneMarker(CENTER_X - ROAD_WIDTH/2 + 2*LANE_WIDTH + LANE_WIDTH/2,
                  CENTER_Y - ROAD_WIDTH/2 - 30,
                  "A3", {50, 205, 50, 200}, true);

    // --- B1 Lane Indicator (East, Incoming) ---
    drawLaneMarker(CENTER_X + ROAD_WIDTH/2 + 30,
                  CENTER_Y - ROAD_WIDTH/2 + LANE_WIDTH/2,
                  "B1", {100, 150, 255, 200}, false);

    // --- B2 Lane Indicator (East, Normal) ---
    drawLaneMarker(CENTER_X + ROAD_WIDTH/2 + 30,
                  CENTER_Y - ROAD_WIDTH/2 + LANE_WIDTH + LANE_WIDTH/2,
                  "B2", {218, 165, 32, 200}, false);

    // --- B3 Lane Indicator (East, Free) ---
    drawLaneMarker(CENTER_X + ROAD_WIDTH/2 + 30,
                  CENTER_Y - ROAD_WIDTH/2 + 2*LANE_WIDTH + LANE_WIDTH/2,
                  "B3", {50, 205, 50, 200}, false);

    // --- C1 Lane Indicator (South, Incoming) ---
    drawLaneMarker(CENTER_X + LANE_WIDTH/2,
                  CENTER_Y + ROAD_WIDTH/2 + 30,
                  "C1", {100, 150, 255, 200}, true);

    // --- C2 Lane Indicator (South, Normal) ---
    drawLaneMarker(CENTER_X - LANE_WIDTH/2,
                  CENTER_Y + ROAD_WIDTH/2 + 30,
                  "C2", {210, 105, 30, 200}, true);

    // --- C3 Lane Indicator (South, Free) ---
    drawLaneMarker(CENTER_X - 3*LANE_WIDTH/2,
                  CENTER_Y + ROAD_WIDTH/2 + 30,
                  "C3", {50, 205, 50, 200}, true);

    // --- D1 Lane Indicator (West, Incoming) ---
    drawLaneMarker(CENTER_X - ROAD_WIDTH/2 - 30,
                  CENTER_Y + LANE_WIDTH/2,
                  "D1", {100, 150, 255, 200}, false);

    // --- D2 Lane Indicator (West, Normal) ---
    drawLaneMarker(CENTER_X - ROAD_WIDTH/2 - 30,
                  CENTER_Y - LANE_WIDTH/2,
                  "D2", {205, 133, 63, 200}, false);

    // --- D3 Lane Indicator (West, Free) ---
    drawLaneMarker(CENTER_X - ROAD_WIDTH/2 - 30,
                  CENTER_Y - 3*LANE_WIDTH/2,
                  "D3", {50, 205, 50, 200}, false);
}

void Renderer::drawLaneMarker(int x, int y, const std::string& label, SDL_Color color, bool isVertical) {
    const int MARKER_WIDTH = isVertical ? 30 : 20;
    const int MARKER_HEIGHT = isVertical ? 20 : 30;

    // Draw hexagonal background
    const int HEX_SIDES = 6;
    const float HEX_RADIUS = isVertical ? MARKER_WIDTH/2.0f + 2.0f : MARKER_HEIGHT/2.0f + 2.0f;

    // Create polygon points for hexagon
    SDL_FPoint hexPoints[HEX_SIDES + 1];
    for (int i = 0; i < HEX_SIDES; i++) {
        float angle = 2.0f * M_PI * static_cast<float>(i) / static_cast<float>(HEX_SIDES) - M_PI/2.0f;
        hexPoints[i].x = static_cast<float>(x) + HEX_RADIUS * cosf(angle);
        hexPoints[i].y = static_cast<float>(y) + HEX_RADIUS * sinf(angle);
    }
    // Close the polygon
    hexPoints[HEX_SIDES] = hexPoints[0];

    // Draw hexagon with glow effect
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 50); // Transparent glow
    for (int i = 1; i <= 5; i++) {
        float scale = 1.0f + static_cast<float>(i) * 0.08f;
        SDL_FPoint scaledPoints[HEX_SIDES + 1];

        for (int j = 0; j < HEX_SIDES + 1; j++) {
            scaledPoints[j].x = static_cast<float>(x) + (hexPoints[j].x - static_cast<float>(x)) * scale;
            scaledPoints[j].y = static_cast<float>(y) + (hexPoints[j].y - static_cast<float>(y)) * scale;
        }

        for (int j = 0; j < HEX_SIDES; j++) {
            SDL_RenderLine(renderer,
                          scaledPoints[j].x, scaledPoints[j].y,
                          scaledPoints[j+1].x, scaledPoints[j+1].y);
        }
    }

    // Fill hexagon background
    SDL_SetRenderDrawColor(renderer, color.r/2, color.g/2, color.b/2, 200);
    for (int i = 0; i < HEX_SIDES - 2; i++) {
        SDL_RenderLine(renderer, hexPoints[0].x, hexPoints[0].y,
                      hexPoints[i+1].x, hexPoints[i+1].y);
        SDL_RenderLine(renderer, hexPoints[0].x, hexPoints[0].y,
                      hexPoints[i+2].x, hexPoints[i+2].y);
        SDL_RenderLine(renderer, hexPoints[i+1].x, hexPoints[i+1].y,
                      hexPoints[i+2].x, hexPoints[i+2].y);
    }

    // Draw hexagon border
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    for (int i = 0; i < HEX_SIDES; i++) {
        SDL_RenderLine(renderer,
                      hexPoints[i].x, hexPoints[i].y,
                      hexPoints[i+1].x, hexPoints[i+1].y);
    }

    // Draw label using simplified character drawing
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Draw first character (A, B, C, or D)
    float charX = static_cast<float>(x) - 5.0f;
    float charY = static_cast<float>(y) - 4.0f;

    char firstChar = label[0];
    switch (firstChar) {
        case 'A':
            SDL_RenderLine(renderer, charX, charY+8, charX+5, charY); // Left diagonal
            SDL_RenderLine(renderer, charX+5, charY, charX+10, charY+8); // Right diagonal
            SDL_RenderLine(renderer, charX+2, charY+5, charX+8, charY+5); // Middle bar
            break;
        case 'B':
            SDL_RenderLine(renderer, charX, charY, charX, charY+8); // Vertical
            SDL_RenderLine(renderer, charX, charY, charX+7, charY); // Top
            SDL_RenderLine(renderer, charX+7, charY, charX+9, charY+2); // Top curve
            SDL_RenderLine(renderer, charX+9, charY+2, charX+7, charY+4); // Middle top
            SDL_RenderLine(renderer, charX, charY+4, charX+7, charY+4); // Middle
            SDL_RenderLine(renderer, charX+7, charY+4, charX+9, charY+6); // Middle bottom
            SDL_RenderLine(renderer, charX+9, charY+6, charX+7, charY+8); // Bottom curve
            SDL_RenderLine(renderer, charX+7, charY+8, charX, charY+8); // Bottom
            break;
        case 'C':
            SDL_RenderLine(renderer, charX+9, charY, charX+2, charY); // Top
            SDL_RenderLine(renderer, charX+2, charY, charX, charY+2); // Top curve
            SDL_RenderLine(renderer, charX, charY+2, charX, charY+6); // Left
            SDL_RenderLine(renderer, charX, charY+6, charX+2, charY+8); // Bottom curve
            SDL_RenderLine(renderer, charX+2, charY+8, charX+9, charY+8); // Bottom
            break;
        case 'D':
            SDL_RenderLine(renderer, charX, charY, charX, charY+8); // Vertical
            SDL_RenderLine(renderer, charX, charY, charX+7, charY); // Top
            SDL_RenderLine(renderer, charX+7, charY, charX+9, charY+2); // Top curve
            SDL_RenderLine(renderer, charX+9, charY+2, charX+9, charY+6); // Right
            SDL_RenderLine(renderer, charX+9, charY+6, charX+7, charY+8); // Bottom curve
            SDL_RenderLine(renderer, charX+7, charY+8, charX, charY+8); // Bottom
            break;
    }

    // Draw second character (1, 2, or 3)
    charX = static_cast<float>(x) + 1.0f;
    charY = static_cast<float>(y) - 4.0f;

    char secondChar = label[1];
    switch (secondChar) {
        case '1':
            SDL_RenderLine(renderer, charX+4, charY, charX+4, charY+8); // Vertical
            SDL_RenderLine(renderer, charX+2, charY+2, charX+4, charY); // Top slant
            SDL_RenderLine(renderer, charX+2, charY+8, charX+6, charY+8); // Base
            break;
        case '2':
            SDL_RenderLine(renderer, charX+1, charY+1, charX+4, charY); // Top left curve
            SDL_RenderLine(renderer, charX+4, charY, charX+6, charY+1); // Top right curve
            SDL_RenderLine(renderer, charX+6, charY+1, charX+6, charY+3); // Upper right vertical
            SDL_RenderLine(renderer, charX+6, charY+3, charX+1, charY+8); // Diagonal
            SDL_RenderLine(renderer, charX+1, charY+8, charX+7, charY+8); // Bottom
            break;
        case '3':
            SDL_RenderLine(renderer, charX+1, charY, charX+6, charY); // Top
            SDL_RenderLine(renderer, charX+6, charY, charX+7, charY+2); // Top right curve
            SDL_RenderLine(renderer, charX+7, charY+2, charX+5, charY+4); // Upper middle
            SDL_RenderLine(renderer, charX+3, charY+4, charX+5, charY+4); // Middle
            SDL_RenderLine(renderer, charX+5, charY+4, charX+7, charY+6); // Lower middle
            SDL_RenderLine(renderer, charX+7, charY+6, charX+6, charY+8); // Bottom right curve
            SDL_RenderLine(renderer, charX+6, charY+8, charX+1, charY+8); // Bottom
            break;
    }
}

void Renderer::drawCrosswalks() {
    const int CENTER_X = windowWidth / 2;
    const int CENTER_Y = windowHeight / 2;
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;

    // Modern zebra crossing style
    SDL_SetRenderDrawColor(renderer, 240, 240, 255, 200); // Slight blue-white

    // North crosswalk
    for (int i = 0; i < 9; i++) {
        SDL_FRect stripe = {
            static_cast<float>(CENTER_X - ROAD_WIDTH/2 + 2 + i*18),
            static_cast<float>(CENTER_Y - ROAD_WIDTH/2 - 25),
            12.0f, 25.0f
        };
        SDL_RenderFillRect(renderer, &stripe);

        // Add subtle glow
        SDL_SetRenderDrawColor(renderer, 240, 240, 255, 30); // Transparent glow
        SDL_FRect glow = {
            stripe.x - 2, stripe.y - 2,
            stripe.w + 4, stripe.h + 4
        };
        SDL_RenderFillRect(renderer, &glow);

        SDL_SetRenderDrawColor(renderer, 240, 240, 255, 200); // Reset color
    }

    // South crosswalk
    for (int i = 0; i < 9; i++) {
        SDL_FRect stripe = {
            static_cast<float>(CENTER_X - ROAD_WIDTH/2 + 2 + i*18),
            static_cast<float>(CENTER_Y + ROAD_WIDTH/2),
            12.0f, 25.0f
        };
        SDL_RenderFillRect(renderer, &stripe);

        // Add subtle glow
        SDL_SetRenderDrawColor(renderer, 240, 240, 255, 30); // Transparent glow
        SDL_FRect glow = {
            stripe.x - 2, stripe.y - 2,
            stripe.w + 4, stripe.h + 4
        };
        SDL_RenderFillRect(renderer, &glow);

        SDL_SetRenderDrawColor(renderer, 240, 240, 255, 200); // Reset color
    }

    // East crosswalk
    for (int i = 0; i < 9; i++) {
        SDL_FRect stripe = {
            static_cast<float>(CENTER_X + ROAD_WIDTH/2),
            static_cast<float>(CENTER_Y - ROAD_WIDTH/2 + 2 + i*18),
            25.0f, 12.0f
        };
        SDL_RenderFillRect(renderer, &stripe);

        // Add subtle glow
        SDL_SetRenderDrawColor(renderer, 240, 240, 255, 30); // Transparent glow
        SDL_FRect glow = {
            stripe.x - 2, stripe.y - 2,
            stripe.w + 4, stripe.h + 4
        };
        SDL_RenderFillRect(renderer, &glow);

        SDL_SetRenderDrawColor(renderer, 240, 240, 255, 200); // Reset color
    }

    // West crosswalk
    for (int i = 0; i < 9; i++) {
        SDL_FRect stripe = {
            static_cast<float>(CENTER_X - ROAD_WIDTH/2 - 25),
            static_cast<float>(CENTER_Y - ROAD_WIDTH/2 + 2 + i*18),
            25.0f, 12.0f
        };
        SDL_RenderFillRect(renderer, &stripe);

        // Add subtle glow
        SDL_SetRenderDrawColor(renderer, 240, 240, 255, 30); // Transparent glow
        SDL_FRect glow = {
            stripe.x - 2, stripe.y - 2,
            stripe.w + 4, stripe.h + 4
        };
        SDL_RenderFillRect(renderer, &glow);

        SDL_SetRenderDrawColor(renderer, 240, 240, 255, 200); // Reset color
    }
}

void Renderer::drawStopLines() {
    const int CENTER_X = windowWidth / 2;
    const int CENTER_Y = windowHeight / 2;
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;

    // Draw stop lines with glow effect
    SDL_SetRenderDrawColor(renderer, 240, 240, 255, 255); // Bright white-blue

    // Top stop line (A road)
    SDL_FRect topStop = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2 - 3),
        static_cast<float>(ROAD_WIDTH),
        3.0f
    };
    SDL_RenderFillRect(renderer, &topStop);

    // Add glow
    SDL_SetRenderDrawColor(renderer, 240, 240, 255, 30); // Transparent glow
    SDL_FRect topGlow = {
        topStop.x, topStop.y - 3,
        topStop.w, 9.0f
    };
    SDL_RenderFillRect(renderer, &topGlow);

    // Bottom stop line (C road)
    SDL_SetRenderDrawColor(renderer, 240, 240, 255, 255);
    SDL_FRect bottomStop = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y + ROAD_WIDTH/2),
        static_cast<float>(ROAD_WIDTH),
        3.0f
    };
    SDL_RenderFillRect(renderer, &bottomStop);

    // Add glow
    SDL_SetRenderDrawColor(renderer, 240, 240, 255, 30);
    SDL_FRect bottomGlow = {
        bottomStop.x, bottomStop.y - 3,
        bottomStop.w, 9.0f
    };
    SDL_RenderFillRect(renderer, &bottomGlow);

    // Left stop line (D road)
    SDL_SetRenderDrawColor(renderer, 240, 240, 255, 255);
    SDL_FRect leftStop = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2 - 3),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2),
        3.0f,
        static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &leftStop);

    // Add glow
    SDL_SetRenderDrawColor(renderer, 240, 240, 255, 30);
    SDL_FRect leftGlow = {
        leftStop.x - 3, leftStop.y,
        9.0f, leftStop.h
    };
    SDL_RenderFillRect(renderer, &leftGlow);

    // Right stop line (B road)
    SDL_SetRenderDrawColor(renderer, 240, 240, 255, 255);
    SDL_FRect rightStop = {
        static_cast<float>(CENTER_X + ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2),
        3.0f,
        static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &rightStop);

    // Add glow
    SDL_SetRenderDrawColor(renderer, 240, 240, 255, 30);
    SDL_FRect rightGlow = {
        rightStop.x - 3, rightStop.y,
        9.0f, rightStop.h
    };
    SDL_RenderFillRect(renderer, &rightGlow);
}

void Renderer::drawLaneFlowArrow(int x, int y, Direction dir) {
    // Draw a modern arrow with glow effect
    const float ARROW_SIZE = 20.0f;
    const float ARROW_WIDTH = 10.0f;

    // Glow effect (larger, transparent)
    SDL_SetRenderDrawColor(renderer, 220, 220, 255, 50); // Light blue glow

    // Determine arrow points based on direction
    SDL_FPoint points[7]; // Arrow polygon

    switch (dir) {
        case Direction::UP:
            // Head
            points[0] = {static_cast<float>(x), static_cast<float>(y) - ARROW_SIZE};
            points[1] = {static_cast<float>(x) - ARROW_WIDTH, static_cast<float>(y) - ARROW_SIZE/2};
            points[2] = {static_cast<float>(x) - ARROW_WIDTH/2, static_cast<float>(y) - ARROW_SIZE/2};
            // Stem
            points[3] = {static_cast<float>(x) - ARROW_WIDTH/2, static_cast<float>(y) + ARROW_SIZE/2};
            points[4] = {static_cast<float>(x) + ARROW_WIDTH/2, static_cast<float>(y) + ARROW_SIZE/2};
            // Head right
            points[5] = {static_cast<float>(x) + ARROW_WIDTH/2, static_cast<float>(y) - ARROW_SIZE/2};
            points[6] = {static_cast<float>(x) + ARROW_WIDTH, static_cast<float>(y) - ARROW_SIZE/2};
            break;
        case Direction::DOWN:
            // Head
            points[0] = {static_cast<float>(x), static_cast<float>(y) + ARROW_SIZE};
            points[1] = {static_cast<float>(x) - ARROW_WIDTH, static_cast<float>(y) + ARROW_SIZE/2};
            points[2] = {static_cast<float>(x) - ARROW_WIDTH/2, static_cast<float>(y) + ARROW_SIZE/2};
            // Stem
            points[3] = {static_cast<float>(x) - ARROW_WIDTH/2, static_cast<float>(y) - ARROW_SIZE/2};
            points[4] = {static_cast<float>(x) + ARROW_WIDTH/2, static_cast<float>(y) - ARROW_SIZE/2};
            // Head right
            points[5] = {static_cast<float>(x) + ARROW_WIDTH/2, static_cast<float>(y) + ARROW_SIZE/2};
            points[6] = {static_cast<float>(x) + ARROW_WIDTH, static_cast<float>(y) + ARROW_SIZE/2};
            break;
        case Direction::LEFT:
            // Head
            points[0] = {static_cast<float>(x) - ARROW_SIZE, static_cast<float>(y)};
            points[1] = {static_cast<float>(x) - ARROW_SIZE/2, static_cast<float>(y) - ARROW_WIDTH};
            points[2] = {static_cast<float>(x) - ARROW_SIZE/2, static_cast<float>(y) - ARROW_WIDTH/2};
            // Stem
            points[3] = {static_cast<float>(x) + ARROW_SIZE/2, static_cast<float>(y) - ARROW_WIDTH/2};
            points[4] = {static_cast<float>(x) + ARROW_SIZE/2, static_cast<float>(y) + ARROW_WIDTH/2};
            // Head bottom
            points[5] = {static_cast<float>(x) - ARROW_SIZE/2, static_cast<float>(y) + ARROW_WIDTH/2};
            points[6] = {static_cast<float>(x) - ARROW_SIZE/2, static_cast<float>(y) + ARROW_WIDTH};
            break;
        case Direction::RIGHT:
            // Head
            points[0] = {static_cast<float>(x) + ARROW_SIZE, static_cast<float>(y)};
            points[1] = {static_cast<float>(x) + ARROW_SIZE/2, static_cast<float>(y) - ARROW_WIDTH};
            points[2] = {static_cast<float>(x) + ARROW_SIZE/2, static_cast<float>(y) - ARROW_WIDTH/2};
            // Stem
            points[3] = {static_cast<float>(x) - ARROW_SIZE/2, static_cast<float>(y) - ARROW_WIDTH/2};
            points[4] = {static_cast<float>(x) - ARROW_SIZE/2, static_cast<float>(y) + ARROW_WIDTH/2};
            // Head bottom
            points[5] = {static_cast<float>(x) + ARROW_SIZE/2, static_cast<float>(y) + ARROW_WIDTH/2};
            points[6] = {static_cast<float>(x) + ARROW_SIZE/2, static_cast<float>(y) + ARROW_WIDTH};
            break;
    }

    // Draw glow - slightly larger version of the arrow
    for (int i = 0; i < 7; i++) {
        float scaledX = static_cast<float>(x) + (points[i].x - static_cast<float>(x)) * 1.2f;
        float scaledY = static_cast<float>(y) + (points[i].y - static_cast<float>(y)) * 1.2f;

        // Connect the glow points
        int next = (i + 1) % 7;
        float nextScaledX = static_cast<float>(x) + (points[next].x - static_cast<float>(x)) * 1.2f;
        float nextScaledY = static_cast<float>(y) + (points[next].y - static_cast<float>(y)) * 1.2f;

        SDL_RenderLine(renderer, scaledX, scaledY, nextScaledX, nextScaledY);
    }

    // Draw the actual arrow
    SDL_SetRenderDrawColor(renderer, 220, 220, 255, 200);

    // Create vertices for filled polygon with SDL_FColor for SDL3 compatibility
    SDL_Vertex vertices[7];
    SDL_FColor arrowColor = {
        220.0f/255.0f, 220.0f/255.0f, 255.0f/255.0f, 0.8f
    };

    // Create triangle fan
    for (int i = 0; i < 7; i++) {
        vertices[i].position = points[i];
        vertices[i].color = arrowColor;
    }

    // Draw the filled arrow
    int indices[] = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6};
    SDL_RenderGeometry(renderer, NULL, vertices, 7, indices, 15);

    // Draw outline
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < 7; i++) {
        int next = (i + 1) % 7;
        SDL_RenderLine(renderer, points[i].x, points[i].y, points[next].x, points[next].y);
    }
}

void Renderer::drawLaneLabels() {
    const int CENTER_X = windowWidth / 2;
    const int CENTER_Y = windowHeight / 2;
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;

    // Draw road identifiers with glowing neon-style signs

    // Road A (North) Identifier - Blue neon
    drawNeonSign(CENTER_X, 30, "NORTH", {100, 150, 255, 255}, true);

    // Road B (East) Identifier - Purple neon
    drawNeonSign(windowWidth - 30, CENTER_Y, "EAST", {180, 100, 255, 255}, false);

    // Road C (South) Identifier - Orange neon
    drawNeonSign(CENTER_X, windowHeight - 30, "SOUTH", {255, 150, 100, 255}, true);

    // Road D (West) Identifier - Green neon
    drawNeonSign(30, CENTER_Y, "WEST", {100, 255, 150, 255}, false);

    // Draw lane classification legend in a modern style
}


void Renderer::cleanup() {
    // Add cleanup code here
}

void Renderer::toggleDebugOverlay() {
    // Add toggle logic here
}

void Renderer::drawText(const std::string& text, int x, int y, SDL_Color color) {
    // Ensure SDL_ttf is initialized and draw the text
}


void Renderer::drawNeonSign(int x, int y, const std::string& text, SDL_Color color, bool isHorizontal) {
    // Draw a neon-style sign with text
    const int SIGN_PADDING = 10;
    const int SIGN_BORDER = 2;
    const int CHAR_WIDTH = 12;
    const int CHAR_HEIGHT = 20;

    // Calculate sign dimensions based on text length
    int textWidth = text.length() * CHAR_WIDTH;
    int signWidth = textWidth + 2 * SIGN_PADDING;
    int signHeight = CHAR_HEIGHT + 2 * SIGN_PADDING;

    float signX = isHorizontal ? static_cast<float>(x - signWidth/2) : static_cast<float>(x - signHeight/2);
    float signY = isHorizontal ? static_cast<float>(y - signHeight/2) : static_cast<float>(y - signWidth/2);

    // Draw outer glow
    for (int i = 1; i <= 5; i++) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255/(i*3));

        if (isHorizontal) {
            SDL_FRect glow = {
                signX - static_cast<float>(i), signY - static_cast<float>(i),
                static_cast<float>(signWidth + 2*i), static_cast<float>(signHeight + 2*i)
            };
            SDL_RenderRect(renderer, &glow);
        } else {
            // Rotated 90 degrees for vertical sign
            SDL_FRect glow = {
                signX - static_cast<float>(i), signY - static_cast<float>(i),
                static_cast<float>(signHeight + 2*i), static_cast<float>(signWidth + 2*i)
            };
            SDL_RenderRect(renderer, &glow);
        }
    }

    // Draw sign background
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 200);
    if (isHorizontal) {
        SDL_FRect signBg = {
            signX, signY,
            static_cast<float>(signWidth), static_cast<float>(signHeight)
        };
        SDL_RenderFillRect(renderer, &signBg);
    } else {
        SDL_FRect signBg = {
            signX, signY,
            static_cast<float>(signHeight), static_cast<float>(signWidth)
        };
        SDL_RenderFillRect(renderer, &signBg);
    }

    // Draw neon border
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    if (isHorizontal) {
        SDL_FRect signBorder = {
            signX, signY,
            static_cast<float>(signWidth), static_cast<float>(signHeight)
        };
        SDL_RenderRect(renderer, &signBorder);
    } else {
        SDL_FRect signBorder = {
            signX, signY,
            static_cast<float>(signHeight), static_cast<float>(signWidth)
        };
        SDL_RenderRect(renderer, &signBorder);
    }

    // Draw text character by character
    for (size_t i = 0; i < text.length(); i++) {
        float charX, charY;

        if (isHorizontal) {
            charX = signX + static_cast<float>(SIGN_PADDING + i * CHAR_WIDTH);
            charY = signY + static_cast<float>(SIGN_PADDING);
        } else {
            // Vertical text
            charX = signX + static_cast<float>(SIGN_PADDING);
            charY = signY + static_cast<float>(SIGN_PADDING + i * CHAR_WIDTH);
        }

        // Draw character with neon glow
        drawNeonChar(charX, charY, text[i], color, !isHorizontal);
    }
}

void Renderer::drawNeonChar(float x, float y, char c, SDL_Color color, bool isVertical) {
    const float CHAR_WIDTH = 12.0f;
    const float CHAR_HEIGHT = 20.0f;

    // Simplified character drawing with neon effect
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

    // Draw character glow
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Draw different letters with neon style
    switch (c) {
        case 'A':
            // Main lines
            SDL_RenderLine(renderer, x+CHAR_WIDTH/2, y, x, y+CHAR_HEIGHT); // Left diagonal
            SDL_RenderLine(renderer, x+CHAR_WIDTH/2, y, x+CHAR_WIDTH, y+CHAR_HEIGHT); // Right diagonal
            SDL_RenderLine(renderer, x+CHAR_WIDTH/4, y+CHAR_HEIGHT/2, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT/2); // Middle
            break;

        case 'B':
            SDL_RenderLine(renderer, x, y, x, y+CHAR_HEIGHT); // Vertical
            SDL_RenderLine(renderer, x, y, x+3*CHAR_WIDTH/4, y); // Top
            SDL_RenderLine(renderer, x+3*CHAR_WIDTH/4, y, x+CHAR_WIDTH, y+CHAR_HEIGHT/4); // Top curve
            SDL_RenderLine(renderer, x+CHAR_WIDTH, y+CHAR_HEIGHT/4, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT/2); // Middle top
            SDL_RenderLine(renderer, x, y+CHAR_HEIGHT/2, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT/2); // Middle
            SDL_RenderLine(renderer, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT/2, x+CHAR_WIDTH, y+3*CHAR_HEIGHT/4); // Middle bottom
            SDL_RenderLine(renderer, x+CHAR_WIDTH, y+3*CHAR_HEIGHT/4, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT); // Bottom curve
            SDL_RenderLine(renderer, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT, x, y+CHAR_HEIGHT); // Bottom
            break;

        case 'N':
            SDL_RenderLine(renderer, x, y, x, y+CHAR_HEIGHT); // Left vertical
            SDL_RenderLine(renderer, x, y, x+CHAR_WIDTH, y+CHAR_HEIGHT); // Diagonal
            SDL_RenderLine(renderer, x+CHAR_WIDTH, y, x+CHAR_WIDTH, y+CHAR_HEIGHT); // Right vertical
            break;

        case 'O':
            SDL_RenderLine(renderer, x+CHAR_WIDTH/4, y, x+3*CHAR_WIDTH/4, y); // Top
            SDL_RenderLine(renderer, x+3*CHAR_WIDTH/4, y, x+CHAR_WIDTH, y+CHAR_HEIGHT/4); // Top right
            SDL_RenderLine(renderer, x+CHAR_WIDTH, y+CHAR_HEIGHT/4, x+CHAR_WIDTH, y+3*CHAR_HEIGHT/4); // Right
            SDL_RenderLine(renderer, x+CHAR_WIDTH, y+3*CHAR_HEIGHT/4, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT); // Bottom right
            SDL_RenderLine(renderer, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT, x+CHAR_WIDTH/4, y+CHAR_HEIGHT); // Bottom
            SDL_RenderLine(renderer, x+CHAR_WIDTH/4, y+CHAR_HEIGHT, x, y+3*CHAR_HEIGHT/4); // Bottom left
            SDL_RenderLine(renderer, x, y+3*CHAR_HEIGHT/4, x, y+CHAR_HEIGHT/4); // Left
            SDL_RenderLine(renderer, x, y+CHAR_HEIGHT/4, x+CHAR_WIDTH/4, y); // Top left
            break;

        case 'R':
            SDL_RenderLine(renderer, x, y, x, y+CHAR_HEIGHT); // Vertical
            SDL_RenderLine(renderer, x, y, x+3*CHAR_WIDTH/4, y); // Top
            SDL_RenderLine(renderer, x+3*CHAR_WIDTH/4, y, x+CHAR_WIDTH, y+CHAR_HEIGHT/4); // Top curve
            SDL_RenderLine(renderer, x+CHAR_WIDTH, y+CHAR_HEIGHT/4, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT/2); // Middle top
            SDL_RenderLine(renderer, x, y+CHAR_HEIGHT/2, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT/2); // Middle
            SDL_RenderLine(renderer, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT/2, x+CHAR_WIDTH, y+CHAR_HEIGHT); // Diagonal
            break;

        case 'T':
            SDL_RenderLine(renderer, x, y, x+CHAR_WIDTH, y); // Top
            SDL_RenderLine(renderer, x+CHAR_WIDTH/2, y, x+CHAR_WIDTH/2, y+CHAR_HEIGHT); // Vertical
            break;

        case 'E':
            SDL_RenderLine(renderer, x, y, x, y+CHAR_HEIGHT); // Vertical
            SDL_RenderLine(renderer, x, y, x+CHAR_WIDTH, y); // Top
            SDL_RenderLine(renderer, x, y+CHAR_HEIGHT/2, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT/2); // Middle
            SDL_RenderLine(renderer, x, y+CHAR_HEIGHT, x+CHAR_WIDTH, y+CHAR_HEIGHT); // Bottom
            break;

        case 'S':
            SDL_RenderLine(renderer, x+CHAR_WIDTH, y, x+CHAR_WIDTH/4, y); // Top
            SDL_RenderLine(renderer, x+CHAR_WIDTH/4, y, x, y+CHAR_HEIGHT/4); // Top curve
            SDL_RenderLine(renderer, x, y+CHAR_HEIGHT/4, x+CHAR_WIDTH/4, y+CHAR_HEIGHT/2); // Middle top
            SDL_RenderLine(renderer, x+CHAR_WIDTH/4, y+CHAR_HEIGHT/2, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT/2); // Middle
            SDL_RenderLine(renderer, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT/2, x+CHAR_WIDTH, y+3*CHAR_HEIGHT/4); // Middle bottom
            SDL_RenderLine(renderer, x+CHAR_WIDTH, y+3*CHAR_HEIGHT/4, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT); // Bottom curve
            SDL_RenderLine(renderer, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT, x, y+CHAR_HEIGHT); // Bottom
            break;

        case 'W':
            SDL_RenderLine(renderer, x, y, x+CHAR_WIDTH/4, y+CHAR_HEIGHT); // Left diagonal
            SDL_RenderLine(renderer, x+CHAR_WIDTH/4, y+CHAR_HEIGHT, x+CHAR_WIDTH/2, y+CHAR_HEIGHT/2); // Middle left
            SDL_RenderLine(renderer, x+CHAR_WIDTH/2, y+CHAR_HEIGHT/2, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT); // Middle right
            SDL_RenderLine(renderer, x+3*CHAR_WIDTH/4, y+CHAR_HEIGHT, x+CHAR_WIDTH, y); // Right diagonal
            break;

        case 'H':
            SDL_RenderLine(renderer, x, y, x, y+CHAR_HEIGHT); // Left vertical
            SDL_RenderLine(renderer, x+CHAR_WIDTH, y, x+CHAR_WIDTH, y+CHAR_HEIGHT); // Right vertical
            SDL_RenderLine(renderer, x, y+CHAR_HEIGHT/2, x+CHAR_WIDTH, y+CHAR_HEIGHT/2); // Middle
            break;

        // Add more characters as needed
        default:
            // For unknown characters, draw a rectangle
            SDL_FRect charRect = {x, y, CHAR_WIDTH, CHAR_HEIGHT};
            SDL_RenderRect(renderer, &charRect);
    }

    // Draw glow effect
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 50);
    for (int i = 1; i <= 3; i++) {
        SDL_FRect glow = {
            x - static_cast<float>(i), y - static_cast<float>(i),
            CHAR_WIDTH + static_cast<float>(2*i), CHAR_HEIGHT + static_cast<float>(2*i)
        };
        SDL_RenderRect(renderer, &glow);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}



void Renderer::drawTrafficLights() {
    if (!trafficManager) {
        return;
    }

    TrafficLight* trafficLight = trafficManager->getTrafficLight();
    if (!trafficLight) {
        return;
    }

    // Draw traffic lights
    trafficLight->render(renderer);
}

void Renderer::drawVehicles() {
    if (!trafficManager) {
        return;
    }

    // Get all lanes from traffic manager
    const std::vector<Lane*>& lanes = trafficManager->getLanes();

    // Draw vehicles in each lane
    for (Lane* lane : lanes) {
        if (!lane) {
            continue;
        }

        const std::vector<Vehicle*>& vehicles = lane->getVehicles();
        int queuePos = 0;

        for (Vehicle* vehicle : vehicles) {
            if (vehicle) {
                // Render vehicle with modern style
                renderModernVehicle(vehicle, queuePos);
                queuePos++;
            }
        }
    }
}

void Renderer::renderModernVehicle(Vehicle* vehicle, int queuePos) {
    if (!vehicle) return;

    // Create default parameters for vehicle rendering
    vehicle->render(renderer, carTexture, queuePos);

    // Add additional modern effects
    float x = vehicle->getTurnPosX();
    float y = vehicle->getTurnPosY();

    // Get direction from vehicle
    Direction dir = vehicle->getDestination() == Destination::STRAIGHT ? Direction::DOWN : Direction::LEFT;

    // Add headlight/taillight glow
    drawVehicleLights(x, y, vehicle->getLaneNumber(), vehicle->getLane(), dir, vehicle->isTurning(), vehicle->getDestination());
}

void Renderer::drawVehicleLights(float x, float y, int laneNumber, char laneChar,
                               Direction dir, bool isTurning, Destination destination) {
    // Draw headlights/taillights glow based on vehicle direction
    // Get vehicle position and calculate direction based on lane

    // Get heading direction based on lane
    Direction heading;
    switch (laneChar) {
        case 'A': heading = Direction::DOWN; break;  // From North, heading South
        case 'B': heading = Direction::LEFT; break;  // From East, heading West
        case 'C': heading = Direction::UP; break;    // From South, heading North
        case 'D': heading = Direction::RIGHT; break; // From West, heading East
        default: heading = Direction::DOWN; break;
    }

    // Calculate light positions relative to vehicle center
    const float LIGHT_DISTANCE = 10.0f;  // Distance from center to lights
    const float LIGHT_RADIUS = 4.0f;     // Radius of lights

    // Position of headlights/taillights depends on heading
    float frontX1, frontY1, frontX2, frontY2; // Front lights (headlights)
    float backX1, backY1, backX2, backY2;     // Back lights (taillights)

    // Use turn progress for smooth transition during turns
    float turnProgress = 0.0f; // Default value since we don't have access to the vehicle's internal function
    if (isTurning) {
        // Approximate turn progress based on reasonable defaults
        turnProgress = 0.5f; // Default to midway through the turn
    }

    float turnFactor = turnProgress;

    // Adjust headlight positions based on heading
    switch (heading) {
        case Direction::DOWN:
            // Vehicle moving down
            frontX1 = x - 6.0f;
            frontY1 = y + LIGHT_DISTANCE;
            frontX2 = x + 6.0f;
            frontY2 = y + LIGHT_DISTANCE;

            backX1 = x - 6.0f;
            backY1 = y - LIGHT_DISTANCE;
            backX2 = x + 6.0f;
            backY2 = y - LIGHT_DISTANCE;

            // Adjust for turning
            if (isTurning && destination == Destination::LEFT) {
                // Turning left (towards East for a vehicle going South)
                float adjustX = turnFactor * LIGHT_DISTANCE;
                float adjustY = turnFactor * LIGHT_DISTANCE * 0.5f;

                frontX1 += adjustX;
                frontY1 -= adjustY;
                frontX2 += adjustX;
                frontY2 -= adjustY;

                backX1 += adjustX * 0.5f;
                backY1 -= adjustY * 0.5f;
                backX2 += adjustX * 0.5f;
                backY2 -= adjustY * 0.5f;
            }
            break;

        case Direction::UP:
            // Vehicle moving up
            frontX1 = x - 6.0f;
            frontY1 = y - LIGHT_DISTANCE;
            frontX2 = x + 6.0f;
            frontY2 = y - LIGHT_DISTANCE;

            backX1 = x - 6.0f;
            backY1 = y + LIGHT_DISTANCE;
            backX2 = x + 6.0f;
            backY2 = y + LIGHT_DISTANCE;

            // Adjust for turning
            if (isTurning && destination == Destination::LEFT) {
                // Turning left (towards West for a vehicle going North)
                float adjustX = turnFactor * LIGHT_DISTANCE;
                float adjustY = turnFactor * LIGHT_DISTANCE * 0.5f;

                frontX1 -= adjustX;
                frontY1 -= adjustY;
                frontX2 -= adjustX;
                frontY2 -= adjustY;

                backX1 -= adjustX * 0.5f;
                backY1 -= adjustY * 0.5f;
                backX2 -= adjustX * 0.5f;
                backY2 -= adjustY * 0.5f;
            }
            break;

        case Direction::LEFT:
            // Vehicle moving left
            frontX1 = x - LIGHT_DISTANCE;
            frontY1 = y - 6.0f;
            frontX2 = x - LIGHT_DISTANCE;
            frontY2 = y + 6.0f;

            backX1 = x + LIGHT_DISTANCE;
            backY1 = y - 6.0f;
            backX2 = x + LIGHT_DISTANCE;
            backY2 = y + 6.0f;

            // Adjust for turning
            if (isTurning && destination == Destination::LEFT) {
                // Turning left (towards South for a vehicle going West)
                float adjustX = turnFactor * LIGHT_DISTANCE * 0.5f;
                float adjustY = turnFactor * LIGHT_DISTANCE;

                frontX1 += adjustX;
                frontY1 += adjustY;
                frontX2 += adjustX;
                frontY2 += adjustY;

                backX1 += adjustX * 0.5f;
                backY1 += adjustY * 0.5f;
                backX2 += adjustX * 0.5f;
                backY2 += adjustY * 0.5f;
            }
            break;

        case Direction::RIGHT:
            // Vehicle moving right
            frontX1 = x + LIGHT_DISTANCE;
            frontY1 = y - 6.0f;
            frontX2 = x + LIGHT_DISTANCE;
            frontY2 = y + 6.0f;

            backX1 = x - LIGHT_DISTANCE;
            backY1 = y - 6.0f;
            backX2 = x - LIGHT_DISTANCE;
            backY2 = y + 6.0f;

            // Adjust for turning
            if (isTurning && destination == Destination::LEFT) {
                // Turning left (towards North for a vehicle going East)
                float adjustX = turnFactor * LIGHT_DISTANCE * 0.5f;
                float adjustY = turnFactor * LIGHT_DISTANCE;

                frontX1 -= adjustX;
                frontY1 -= adjustY;
                frontX2 -= adjustX;
                frontY2 -= adjustY;

                backX1 -= adjustX * 0.5f;
                backY1 -= adjustY * 0.5f;
                backX2 -= adjustX * 0.5f;
                backY2 -= adjustY * 0.5f;
            }
            break;
    }

    // Draw headlights (front lights) - white/yellow glow
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Inner bright glow
    SDL_SetRenderDrawColor(renderer, 255, 255, 220, 200);
    SDL_FRect headlight1 = {
        frontX1 - LIGHT_RADIUS/2, frontY1 - LIGHT_RADIUS/2,
        LIGHT_RADIUS, LIGHT_RADIUS
    };
    SDL_RenderFillRect(renderer, &headlight1);

    SDL_FRect headlight2 = {
        frontX2 - LIGHT_RADIUS/2, frontY2 - LIGHT_RADIUS/2,
        LIGHT_RADIUS, LIGHT_RADIUS
    };
    SDL_RenderFillRect(renderer, &headlight2);

    // Outer headlight glow
    for (int i = 1; i <= 3; i++) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 220, 200/(i*2));

        SDL_FRect headGlow1 = {
            frontX1 - LIGHT_RADIUS/2 - static_cast<float>(i),
            frontY1 - LIGHT_RADIUS/2 - static_cast<float>(i),
            LIGHT_RADIUS + static_cast<float>(2*i),
            LIGHT_RADIUS + static_cast<float>(2*i)
        };
        SDL_RenderFillRect(renderer, &headGlow1);

        SDL_FRect headGlow2 = {
            frontX2 - LIGHT_RADIUS/2 - static_cast<float>(i),
            frontY2 - LIGHT_RADIUS/2 - static_cast<float>(i),
            LIGHT_RADIUS + static_cast<float>(2*i),
            LIGHT_RADIUS + static_cast<float>(2*i)
        };
        SDL_RenderFillRect(renderer, &headGlow2);
    }

    // Draw taillights (back lights) - red glow
    SDL_SetRenderDrawColor(renderer, 255, 60, 60, 200);
    SDL_FRect taillight1 = {
        backX1 - LIGHT_RADIUS/2, backY1 - LIGHT_RADIUS/2,
        LIGHT_RADIUS, LIGHT_RADIUS
    };
    SDL_RenderFillRect(renderer, &taillight1);

    SDL_FRect taillight2 = {
        backX2 - LIGHT_RADIUS/2, backY2 - LIGHT_RADIUS/2,
        LIGHT_RADIUS, LIGHT_RADIUS
    };
    SDL_RenderFillRect(renderer, &taillight2);

    // Outer taillight glow
    for (int i = 1; i <= 2; i++) {
        SDL_SetRenderDrawColor(renderer, 255, 60, 60, 200/(i*2));

        SDL_FRect tailGlow1 = {
            backX1 - LIGHT_RADIUS/2 - static_cast<float>(i),
            backY1 - LIGHT_RADIUS/2 - static_cast<float>(i),
            LIGHT_RADIUS + static_cast<float>(2*i),
            LIGHT_RADIUS + static_cast<float>(2*i)
        };
        SDL_RenderFillRect(renderer, &tailGlow1);

        SDL_FRect tailGlow2 = {
            backX2 - LIGHT_RADIUS/2 - static_cast<float>(i),
            backY2 - LIGHT_RADIUS/2 - static_cast<float>(i),
            LIGHT_RADIUS + static_cast<float>(2*i),
            LIGHT_RADIUS + static_cast<float>(2*i)
        };
        SDL_RenderFillRect(renderer, &tailGlow2);
    }

    // If vehicle is turning left, draw turn signal
    if (destination == Destination::LEFT) {
        // Determine blink timing using milliseconds
        uint32_t time = SDL_GetTicks();
        bool blinkOn = (time / 500) % 2 == 0; // Blink every 500ms

        if (blinkOn) {
            // Left turn signal - amber/yellow glow
            SDL_SetRenderDrawColor(renderer, 255, 180, 0, 200);

            // Position depends on heading direction
            float turnX, turnY;
            switch (heading) {
                case Direction::DOWN:
                    turnX = x - 8.0f;
                    turnY = y + LIGHT_DISTANCE;
                    break;
                case Direction::UP:
                    turnX = x + 8.0f;
                    turnY = y - LIGHT_DISTANCE;
                    break;
                case Direction::LEFT:
                    turnX = x - LIGHT_DISTANCE;
                    turnY = y + 8.0f;
                    break;
                case Direction::RIGHT:
                    turnX = x + LIGHT_DISTANCE;
                    turnY = y - 8.0f;
                    break;
            }

            // Draw turn signal
            SDL_FRect turnSignal = {
                turnX - LIGHT_RADIUS/2, turnY - LIGHT_RADIUS/2,
                LIGHT_RADIUS, LIGHT_RADIUS
            };
            SDL_RenderFillRect(renderer, &turnSignal);

            // Outer turn signal glow
            for (int i = 1; i <= 3; i++) {
                SDL_SetRenderDrawColor(renderer, 255, 180, 0, 200/(i*2));

                SDL_FRect turnGlow = {
                    turnX - LIGHT_RADIUS/2 - static_cast<float>(i),
                    turnY - LIGHT_RADIUS/2 - static_cast<float>(i),
                    LIGHT_RADIUS + static_cast<float>(2*i),
                    LIGHT_RADIUS + static_cast<float>(2*i)
                };
                SDL_RenderFillRect(renderer, &turnGlow);
            }
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Renderer::drawDebugOverlay() {
    // Draw a modern glass-style debug overlay

    // Draw semi-transparent glass panel with glow effect
    SDL_SetRenderDrawColor(renderer, 20, 25, 40, 200); // Dark blue-ish background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Main panel
    SDL_FRect panelRect = {
        static_cast<float>(windowWidth - 310),
        10.0f,
        300.0f,
        180.0f
    };
    SDL_RenderFillRect(renderer, &panelRect);

    // Panel highlight (top-left edge glow)
    SDL_SetRenderDrawColor(renderer, 100, 140, 200, 100);
    SDL_FRect highlight = {
        panelRect.x,
        panelRect.y,
        panelRect.w,
        2.0f
    };
    SDL_RenderFillRect(renderer, &highlight);

    SDL_FRect highlightSide = {
        panelRect.x,
        panelRect.y,
        2.0f,
        panelRect.h
    };
    SDL_RenderFillRect(renderer, &highlightSide);

    // Panel shadow (bottom-right edge)
    SDL_SetRenderDrawColor(renderer, 10, 15, 30, 150);
    SDL_FRect shadow = {
        panelRect.x,
        panelRect.y + panelRect.h - 2.0f,
        panelRect.w,
        2.0f
    };
    SDL_RenderFillRect(renderer, &shadow);

    SDL_FRect shadowSide = {
        panelRect.x + panelRect.w - 2.0f,
        panelRect.y,
        2.0f,
        panelRect.h
    };
    SDL_RenderFillRect(renderer, &shadowSide);

    // Panel border with glow
    SDL_SetRenderDrawColor(renderer, 100, 140, 200, 255);
    SDL_RenderRect(renderer, &panelRect);

    // Add outer glow effect
    for (int i = 1; i <= 3; i++) {
        SDL_SetRenderDrawColor(renderer, 100, 140, 200, 100/i);
        SDL_FRect glowRect = {
            panelRect.x - static_cast<float>(i),
            panelRect.y - static_cast<float>(i),
            panelRect.w + static_cast<float>(2*i),
            panelRect.h + static_cast<float>(2*i)
        };
        SDL_RenderRect(renderer, &glowRect);
    }

    // Draw panel title
    SDL_SetRenderDrawColor(renderer, 220, 240, 255, 255);
    drawNeonSign(windowWidth - 160, 20, "TRAFFIC STATS", {220, 240, 255, 255}, true);

    // Draw statistics
    drawStatistics();

    // Draw keyboard hint at bottom
    SDL_SetRenderDrawColor(renderer, 180, 200, 255, 200);
    float keyX = windowWidth - 290;
    float keyY = panelRect.y + panelRect.h - 30;

    // Key background
    SDL_FRect keyRect = {
        keyX,
        keyY,
        20.0f,
        20.0f
    };
    SDL_RenderFillRect(renderer, &keyRect);
    SDL_SetRenderDrawColor(renderer, 100, 140, 200, 255);
    SDL_RenderRect(renderer, &keyRect);

    // Key letter
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    // Draw 'D'
    SDL_RenderLine(renderer, keyX + 5, keyY + 4, keyX + 5, keyY + 16); // Vertical
    SDL_RenderLine(renderer, keyX + 5, keyY + 4, keyX + 12, keyY + 4); // Top
    SDL_RenderLine(renderer, keyX + 12, keyY + 4, keyX + 15, keyY + 7); // Top curve
    SDL_RenderLine(renderer, keyX + 15, keyY + 7, keyX + 15, keyY + 13); // Right
    SDL_RenderLine(renderer, keyX + 15, keyY + 13, keyX + 12, keyY + 16); // Bottom curve
    SDL_RenderLine(renderer, keyX + 12, keyY + 16, keyX + 5, keyY + 16); // Bottom

    // Key hint text
    drawText("Toggle debug overlay", keyX + 25, keyY + 3, {220, 240, 255, 255});

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// Replace your entire drawStatistics() method with this implementation
void Renderer::drawStatistics() {
    if (!trafficManager) {
        return;
    }

    // Get statistics from traffic manager with modern layout
    std::string stats = trafficManager->getStatistics();

    // Split into lines
    std::istringstream stream(stats);
    std::string line;
    int y = 50;

    // Draw statistics with modern styling
    while (std::getline(stream, line)) {
        if (line.find("Lane Statistics") != std::string::npos) {
            // Section header - bright blue
            drawText(line, windowWidth - 290, y, {160, 200, 255, 255});
        }
        else if (line.find("Total") != std::string::npos) {
            // Total vehicle count - bright white
            drawText(line, windowWidth - 290, y, {255, 255, 255, 255});
        }
        else if (line.find("A2") != std::string::npos) {
            // Priority lane A2 - orange with pulsing effect
            uint32_t time = SDL_GetTicks();
            int pulse = static_cast<int>(30 * sin(time * 0.003) + 225);
            drawText(line, windowWidth - 290, y, {255, static_cast<Uint8>(pulse), 0, 255});
        }
        else if (line.find("PRIORITY") != std::string::npos) {
            // Priority mode active - flashing orange
            uint32_t time = SDL_GetTicks();
            bool flash = (time / 500) % 2 == 0;
            SDL_Color color = flash ? SDL_Color{255, 180, 0, 255} : SDL_Color{255, 120, 0, 255};
            drawText(line, windowWidth - 290, y, color);
        }
        else {
            // Regular lines - white
            drawText(line, windowWidth - 290, y, {220, 220, 255, 255});
        }
        y += 20;
    }

    // Show current traffic light state
    TrafficLight* trafficLight = trafficManager->getTrafficLight();
    if (trafficLight) {
        y += 10; // Extra space
        std::string stateStr = "Traffic Light: ";
        SDL_Color stateColor;

        auto currentState = trafficLight->getCurrentState();
        switch (currentState) {
            case TrafficLight::State::ALL_RED:
                stateStr += "All Red";
                stateColor = {255, 100, 100, 255};
                break;
            case TrafficLight::State::A_GREEN:
                stateStr += "A Green (North)";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::B_GREEN:
                stateStr += "B Green (East)";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::C_GREEN:
                stateStr += "C Green (South)";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::D_GREEN:
                stateStr += "D Green (West)";
                stateColor = {100, 255, 100, 255};
                break;
        }

        // Pulsing effect for green lights
        if (currentState != TrafficLight::State::ALL_RED) {
            uint32_t time = SDL_GetTicks();
            int pulse = static_cast<int>(40 * sin(time * 0.005) + 215);
            stateColor.g = pulse;
        }

        drawText(stateStr, windowWidth - 290, y, stateColor);
    }
}