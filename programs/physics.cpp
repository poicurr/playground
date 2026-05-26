// physics.cpp - Box2D + Canvas terminal physics playground
// Simple 2D physics sandbox rendered in the terminal.

#include <atomic>
#include <csignal>
#include <cmath>
#include <thread>
#include <vector>

#include "Canvas.hpp"
#include "Random.hpp"

#ifdef _WIN32
#include "TerminalWindows.hpp"
#else
#include "TerminalLinux.hpp"
#endif

#include <box2d/box2d.h>

std::atomic<bool> g_shutdown{false};
void handleSigint(int) { g_shutdown.store(true, std::memory_order_relaxed); }

struct BodyDrawInfo {
    b2BodyId bodyId;
    Cell cell;
    float halfWidth;
    float halfHeight; // for boxes
    float radius;     // for circles
    bool isCircle;
};

int main() {
    std::signal(SIGINT, handleSigint);

#ifdef _WIN32
    auto term = std::make_unique<TerminalWindows>();
#else
    auto term = std::make_unique<TerminalLinux>();
#endif

    Canvas canvas(term.get());
    term->enableMouse(true);
    Random rng;

    // --- Box2D world setup (v3 API) ---
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, -20.0f};
    b2WorldId worldId = b2CreateWorld(&worldDef);

    std::vector<BodyDrawInfo> bodies;

    // Ground (static)
    {
        b2BodyDef groundDef = b2DefaultBodyDef();
        groundDef.position = {0.0f, -1.0f};
        b2BodyId ground = b2CreateBody(worldId, &groundDef);

        b2Polygon groundBox = b2MakeBox(50.0f, 1.0f);
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        b2CreatePolygonShape(ground, &shapeDef, &groundBox);

        bodies.push_back(BodyDrawInfo{
            ground, Cell{'#', {80, 80, 80}, {40, 40, 40}},
            50.0f, 1.0f, 0.0f, false
        });
    }

    // Some dynamic boxes
    for (int i = 0; i < 12; ++i) {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = {range(rng, -15.0f, 15.0f), range(rng, 5.0f, 25.0f)};
        bodyDef.rotation = b2MakeRot(range(rng, -0.8f, 0.8f));
        b2BodyId body = b2CreateBody(worldId, &bodyDef);

        float hx = range(rng, 0.6f, 1.8f);
        float hy = range(rng, 0.5f, 1.2f);
        b2Polygon box = b2MakeBox(hx, hy);
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.3f;
        b2CreatePolygonShape(body, &shapeDef, &box);

        Cell c;
        c.ch = '#';
        c.fg = {200 + int(range(rng, -40, 55)), 140 + int(range(rng, -30, 50)), 60};
        c.bg = {30, 20, 10};

        bodies.push_back(BodyDrawInfo{body, c, hx, hy, 0.0f, false});
    }

    // Some circles
    for (int i = 0; i < 8; ++i) {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = {range(rng, -18.0f, 18.0f), range(rng, 8.0f, 22.0f)};
        b2BodyId body = b2CreateBody(worldId, &bodyDef);

        float r = range(rng, 0.6f, 1.4f);
        b2Circle circle = {{0.0f, 0.0f}, r};
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 0.8f;
        shapeDef.material.friction = 0.2f;
        shapeDef.material.restitution = 0.4f;
        b2CreateCircleShape(body, &shapeDef, &circle);

        Cell c;
        c.ch = 'o';
        c.fg = {100, 180, 255};
        c.bg = {10, 30, 60};

        bodies.push_back(BodyDrawInfo{body, c, 0.0f, 0.0f, r, true});
    }

    // Simple world-to-screen mapping
    const float WORLD_LEFT   = -25.0f;
    const float WORLD_RIGHT  =  25.0f;
    const float WORLD_BOTTOM = -2.0f;
    const float WORLD_TOP    =  30.0f;

    auto worldToScreen = [&](float wx, float wy) -> std::pair<int, int> {
        int sx = static_cast<int>((wx - WORLD_LEFT) / (WORLD_RIGHT - WORLD_LEFT) * canvas.width());
        int sy = static_cast<int>((WORLD_TOP - wy) / (WORLD_TOP - WORLD_BOTTOM) * canvas.height());
        return {sx, sy};
    };

    float accumulator = 0.0f;
    const float timeStep = 1.0f / 60.0f;

    // Mouse joint (for grabbing bodies)
    b2JointId mouseJoint = b2_nullJointId;
    b2BodyId grabbedBody = b2_nullBodyId;

    while (!g_shutdown.load(std::memory_order_relaxed)) {
        canvas.resizeToTerminal();

        // Physics step
        accumulator += 0.016f; // rough 60fps target
        while (accumulator >= timeStep) {
            b2World_Step(worldId, timeStep, 4);
            accumulator -= timeStep;
        }

        // --- Mouse drag support ---
        Terminal::MouseState ms;
        if (term->pollMouse(ms)) {
            // Convert screen mouse to world coordinates
            float worldX = WORLD_LEFT + (ms.x + 0.5f) / float(canvas.width()) * (WORLD_RIGHT - WORLD_LEFT);
            float worldY = WORLD_TOP - (ms.y + 0.5f) / float(canvas.height()) * (WORLD_TOP - WORLD_BOTTOM);

            if (ms.left && B2_IS_NULL(mouseJoint)) {
                // Find closest dynamic body to click
                float bestDist = 999999.0f;
                b2BodyId bestBody = b2_nullBodyId;

                for (const auto& info : bodies) {
                    if (b2Body_GetType(info.bodyId) != b2_dynamicBody) continue;

                    b2Vec2 p = b2Body_GetPosition(info.bodyId);
                    float dx = p.x - worldX;
                    float dy = p.y - worldY;
                    float dist = dx*dx + dy*dy;

                    if (dist < bestDist) {
                        bestDist = dist;
                        bestBody = info.bodyId;
                    }
                }

                if (!B2_IS_NULL(bestBody)) {
                    b2MouseJointDef jd = b2DefaultMouseJointDef();
                    // Use the first static body we can find as the "ground" for the mouse joint
                    b2BodyId staticAnchor = b2_nullBodyId;
                    for (const auto& info : bodies) {
                        if (b2Body_GetType(info.bodyId) == b2_staticBody) {
                            staticAnchor = info.bodyId;
                            break;
                        }
                    }
                    if (B2_IS_NULL(staticAnchor)) staticAnchor = bestBody; // fallback

                    jd.bodyIdA = staticAnchor;
                    jd.bodyIdB = bestBody;
                    jd.target = {worldX, worldY};
                    jd.hertz = 5.0f;
                    jd.dampingRatio = 0.7f;
                    mouseJoint = b2CreateMouseJoint(worldId, &jd);
                    grabbedBody = bestBody;
                }
            }

            if (!B2_IS_NULL(mouseJoint)) {
                b2MouseJoint_SetTarget(mouseJoint, {worldX, worldY});

                if (!ms.left) {
                    // Released
                    b2DestroyJoint(mouseJoint);
                    mouseJoint = b2_nullJointId;
                    grabbedBody = b2_nullBodyId;
                }
            }
        }

        // Render
        canvas.clear(Cell{' ', {}, {15, 15, 18}});

        for (const auto& info : bodies) {
            b2Vec2 pos = b2Body_GetPosition(info.bodyId);
            b2Rot rot = b2Body_GetRotation(info.bodyId);
            float angle = b2Rot_GetAngle(rot);

            auto [cx, cy] = worldToScreen(pos.x, pos.y);

            if (info.isCircle) {
                // Simple circle approximation
                int r = std::max(1, static_cast<int>(info.radius * 1.8f));
                for (int dy = -r; dy <= r; ++dy) {
                    for (int dx = -r; dx <= r; ++dx) {
                        if (dx*dx + dy*dy <= r*r) {
                            canvas.put(cx + dx, cy + dy, info.cell.ch, info.cell.fg, info.cell.bg);
                        }
                    }
                }
            } else {
                // Rotated box approximation (draw filled rect using corners)
                float c = std::cos(angle);
                float s = std::sin(angle);

                float hx = info.halfWidth;
                float hy = info.halfHeight;

                // 4 corners in world space
                b2Vec2 corners[4] = {
                    { pos.x + c*hx - s*hy, pos.y + s*hx + c*hy },
                    { pos.x - c*hx - s*hy, pos.y - s*hx + c*hy },
                    { pos.x - c*hx + s*hy, pos.y - s*hx - c*hy },
                    { pos.x + c*hx + s*hy, pos.y + s*hx - c*hy },
                };

                // Find screen bounding box
                int minX = canvas.width(), maxX = 0;
                int minY = canvas.height(), maxY = 0;

                for (auto& corner : corners) {
                    auto [sx, sy] = worldToScreen(corner.x, corner.y);
                    minX = std::min(minX, sx);
                    maxX = std::max(maxX, sx);
                    minY = std::min(minY, sy);
                    maxY = std::max(maxY, sy);
                }

                // Very rough filled rect (good enough for terminal)
                for (int y = minY; y <= maxY; ++y) {
                    for (int x = minX; x <= maxX; ++x) {
                        canvas.put(x, y, info.cell.ch, info.cell.fg, info.cell.bg);
                    }
                }
            }
        }

        // Simple info
        canvas.text(2, 1, "Box2D + Canvas  |  Left drag to grab & throw  |  ctrl-c to quit", {200, 200, 200}, {});

        canvas.present();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    b2DestroyWorld(worldId);
    return 0;
}
