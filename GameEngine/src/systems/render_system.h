#pragma once

#include <entt/fwd.hpp>
#include <raylib.h>
#include <memory>

class SpriteBatch;

class RenderSystem {
private:
    Camera2D camera;
    std::unique_ptr<SpriteBatch> spriteBatch;

protected:
    bool testMode = false;
    bool enabled = true;

public:
    RenderSystem();
    virtual ~RenderSystem();

    void initialize();
    void update(entt::registry& registry);
    void shutdown();

    void setCamera2D(Camera2D& camera);
    virtual void beginCamera();
    virtual void endCamera();

    void setTestMode(bool enabled) { testMode = enabled; }

    // Get sprite batch for external use
    SpriteBatch* getSpriteBatch() const { return spriteBatch.get(); }

    // Camera control methods
    void setCameraTarget(float x, float y);
    void setCameraOffset(float x, float y);
    void setCameraRotation(float rotation);
    void setCameraZoom(float zoom);
    Camera2D getCamera() const;

    // Enable/disable methods
    bool isEnabled() const;
    void setEnabled(bool value);
};
