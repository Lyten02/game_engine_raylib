#include "raylib.h"

int main() {
    InitWindow(100, 100, "Test Sprite Creator");

    Image img = GenImageColor(64, 64, WHITE);
    ExportImage(img, "assets/textures/test_sprite.png");
    UnloadImage(img);

    CloseWindow();
    return 0;
}
