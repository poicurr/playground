// demolition.cpp - Wrecking ball demolition sandbox using Box2D + Canvas
// Swing the ball, destroy the towers. Classic fun.

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

struct RenderBody {
    b2BodyId bodyId;
    Cell cell;
    float halfW, halfH;
    bool isCircle = false;
    float radius = 0.0f;
};

// Very rough but better rotated box drawer
static void drawRotatedBox(Canvas& c, const b2Vec2& pos, float angle,
                           float hx, float hy, const Cell& cell,
                           float worldLeft, float worldRight,
                           float worldTop, float worldBottom) {
    float cA = cosf(angle);
    float sA = sinf(angle);

    // 4 corners
    b2Vec2 corners[4] = {
        { cA * hx - sA * hy,  sA * hx + cA * hy },
        { -cA * hx - sA * hy, -sA * hx + cA * hy },
        { -cA * hx + sA * hy, -sA * hx - cA * hy },
        {  cA * hx + sA * hy,  sA * hx - cA * hy },
    };

    for (auto& p : corners) {
        p.x += pos.x;
        p.y += pos.y;
    }

    auto toScreen = [&](float wx, float wy) {
        int sx = int((wx - worldLeft) / (worldRight - worldLeft) * c.width());
        int sy = int((worldTop - wy) / (worldTop - worldBottom) * c.height());
        return std::pair{sx, sy};
    };

    int minX = c.width(), maxX = -1;
    int minY = c.height(), maxY = -1;

    for (auto& p : corners) {
        auto [sx, sy] = toScreen(p.x, p.y);
        minX = std::min(minX, sx);
        maxX = std::max(maxX, sx);
        minY = std::min(minY, sy);
        maxY = std::max(maxY, sy);
    }

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            c.put(x, y, cell.ch, cell.fg, cell.bg);
        }
    }
}

static void drawCircle(Canvas& c, const b2Vec2& pos, float radius,
                       const Cell& cell,
                       float worldLeft, float worldRight,
                       float worldTop, float worldBottom) {
    auto toScreen = [&](float wx, float wy) {
        int sx = int((wx - worldLeft) / (worldRight - worldLeft) * c.width());
        int sy = int((worldTop - wy) / (worldTop - worldBottom) * c.height());
        return std::pair{sx, sy};
    };

    auto [cx, cy] = toScreen(pos.x, pos.y);
    int r = std::max(1, int(radius * 2.2f));

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (dx*dx + dy*dy <= r*r) {
                c.put(cx + dx, cy + dy, cell.ch, cell.fg, cell.bg);
            }
        }
    }
}

int main() {
    std::signal(SIGINT, handleSigint);

#ifdef _WIN32
    auto term = std::make_unique<TerminalWindows>();
#else
    auto term = std::make_unique<TerminalLinux>();
#endif

    Canvas canvas(term.get());
    term->enableMouse(true);   // motion + buttons via SGR mode

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, -18.0f};
    b2WorldId worldId = b2CreateWorld(&worldDef);

    std::vector<RenderBody> bodies;

    // === Ground ===
    {
        b2BodyDef bd = b2DefaultBodyDef();
        bd.position = {0, -1};
        b2BodyId ground = b2CreateBody(worldId, &bd);
        b2Polygon box = b2MakeBox(60, 1);
        b2ShapeDef sd = b2DefaultShapeDef();
        b2CreatePolygonShape(ground, &sd, &box);

        bodies.push_back({ground, Cell{'#', {70,70,70}, {35,35,35}}, 60, 1, false});
    }

    // === Towers (stacked boxes) ===
    auto makeTower = [&](float baseX, int layers) {
        for (int i = 0; i < layers; ++i) {
            b2BodyDef bd = b2DefaultBodyDef();
            bd.type = b2_dynamicBody;
            bd.position = {baseX, 1.0f + i * 1.9f};
            b2BodyId body = b2CreateBody(worldId, &bd);

            float w = (i % 2 == 0) ? 1.6f : 1.4f;
            b2Polygon shape = b2MakeBox(w, 0.9f);
            b2ShapeDef sd = b2DefaultShapeDef();
            sd.density = 0.9f;
            sd.material.friction = 0.4f;
            b2CreatePolygonShape(body, &sd, &shape);

            uint8_t r = 160 + (i * 7 % 70);
            uint8_t g = 90 + (i * 5 % 50);
            Cell cell{'#', {r, g, 50}, {40, 25, 15}};
            bodies.push_back({body, cell, w, 0.9f, false});
        }
    };

    makeTower(-8.0f, 9);
    makeTower( 0.0f, 12);
    makeTower( 9.0f, 8);

    // === Wrecking Ball (heavy circle on a rope) ===
    b2BodyId anchor;
    b2BodyId ballBody;
    {
        // Anchor point (static)
        b2BodyDef ad = b2DefaultBodyDef();
        ad.position = {0, 22};
        anchor = b2CreateBody(worldId, &ad);

        // Ball
        b2BodyDef bd = b2DefaultBodyDef();
        bd.type = b2_dynamicBody;
        bd.position = {12, 8};
        ballBody = b2CreateBody(worldId, &bd);

        b2Circle circle = {{0,0}, 1.8f};
        b2ShapeDef sd = b2DefaultShapeDef();
        sd.density = 8.0f;           // heavy
        sd.material.friction = 0.3f;
        sd.material.restitution = 0.25f;
        b2CreateCircleShape(ballBody, &sd, &circle);

        // Rope (distance joint)
        b2DistanceJointDef jd = b2DefaultDistanceJointDef();
        jd.bodyIdA = anchor;
        jd.bodyIdB = ballBody;
        jd.localAnchorA = {0, 0};
        jd.localAnchorB = {0, 0};
        jd.length = 14.0f;
        jd.minLength = 13.0f;
        jd.maxLength = 15.5f;
        jd.enableSpring = true;
        jd.hertz = 1.5f;
        jd.dampingRatio = 0.7f;
        b2CreateDistanceJoint(worldId, &jd);

        bodies.push_back({ballBody, Cell{'@', {255, 220, 80}, {80, 50, 20}}, 0, 0, true, 1.8f});
    }

    // World bounds for rendering
    const float WL = -28.0f, WR = 28.0f;
    const float WT = 26.0f,  WB = -2.0f;

    float swingForce = 0.0f;

    while (!g_shutdown.load(std::memory_order_relaxed)) {
        canvas.resizeToTerminal();

        // === Controls ===
        // In a real version we'd have proper input. For now use a simple timer-based swing.
        // You can extend this later with raw input.
        static int frame = 0;
        frame++;

        // Apply swinging force to the ball (simulates player pulling the rope)
        if ((frame / 25) % 4 < 2) {
            b2Body_ApplyForceToCenter(ballBody, {180.0f + swingForce, 40.0f}, true);
        } else {
            b2Body_ApplyForceToCenter(ballBody, {-220.0f - swingForce, 30.0f}, true);
        }

        // Occasional random "extra push" to make it more chaotic
        if (frame % 90 == 0) {
            Random tmpRng;
            b2Body_ApplyLinearImpulseToCenter(ballBody, {range(tmpRng, -80.f, 80.f), -30.0f}, true);
        }

        b2World_Step(worldId, 1.0f/60.0f, 6);

        // --- Mouse interaction ---
        Terminal::MouseState ms;
        if (term->pollMouse(ms)) {
            // Convert screen coords back to world
            float worldX = WL + (ms.x + 0.5f) / canvas.width()  * (WR - WL);
            float worldY = WT - (ms.y + 0.5f) / canvas.height() * (WT - WB);

            if (ms.left) {
                // Left click: strong radial impulse (like hitting with a giant hammer)
                for (auto& rb : bodies) {
                    b2Vec2 p = b2Body_GetPosition(rb.bodyId);
                    b2Vec2 dir = {worldX - p.x, worldY - p.y};
                    float dist2 = dir.x*dir.x + dir.y*dir.y;
                    if (dist2 > 0.1f && dist2 < 180.0f) {
                        float len = std::sqrt(dist2);
                        dir.x /= len; dir.y /= len;
                        float strength = 420.0f / (0.8f + len * 0.15f);
                        b2Vec2 impulse = {dir.x * strength, dir.y * strength};
                        b2Body_ApplyLinearImpulseToCenter(rb.bodyId, impulse, true);
                    }
                }
            }

            if (ms.right) {
                // Right click: attract the wrecking ball toward cursor
                b2Vec2 p = b2Body_GetPosition(ballBody);
                b2Vec2 dir = {worldX - p.x, worldY - p.y};
                float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
                if (len > 0.5f) {
                    dir.x /= len; dir.y /= len;
                    b2Vec2 force = {dir.x * 2800.0f, dir.y * 2800.0f};
                    b2Body_ApplyForceToCenter(ballBody, force, true);
                }
            }
        }

        // Render
        canvas.clear(Cell{' ', {}, {18, 18, 22}});

        for (const auto& rb : bodies) {
            b2Vec2 pos = b2Body_GetPosition(rb.bodyId);
            float ang = b2Rot_GetAngle(b2Body_GetRotation(rb.bodyId));

            if (rb.isCircle) {
                drawCircle(canvas, pos, rb.radius, rb.cell, WL, WR, WT, WB);
            } else {
                drawRotatedBox(canvas, pos, ang, rb.halfW, rb.halfH, rb.cell, WL, WR, WT, WB);
            }
        }

        // Rope visualization (simple line from anchor to ball)
        {
            b2Vec2 a = b2Body_GetPosition(anchor);
            b2Vec2 b = b2Body_GetPosition(ballBody);
            // very crude line
            auto toScreen = [&](float wx, float wy) {
                return std::pair{
                    int((wx - WL) / (WR - WL) * canvas.width()),
                    int((WT - wy) / (WT - WB) * canvas.height())
                };
            };
            auto [x1, y1] = toScreen(a.x, a.y);
            auto [x2, y2] = toScreen(b.x, b.y);
            // draw a few points along the line
            for (int i = 0; i < 12; ++i) {
                float t = i / 11.0f;
                int lx = int(x1 + (x2 - x1) * t);
                int ly = int(y1 + (y2 - y1) * t);
                canvas.put(lx, ly, '.', {200, 180, 120}, {});
            }
        }

        // UI
        canvas.text(2, 1, "DEMOLITION  |  Wrecking Ball + Mouse  |  ctrl-c to quit", {255, 230, 150}, {});
        canvas.text(2, 2, "Left click = smash nearby bodies   |   Right click = pull the ball toward cursor", {120, 120, 120}, {});

        canvas.present();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    b2DestroyWorld(worldId);
    return 0;
}
