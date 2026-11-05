#pragma once


namespace Input {

struct State {
    float tilt = 0.0f;          // -1.0 (left) to +1.0 (right)
    bool jumpHold = false;      // Wiimote 2 / A, or both shoulders
    bool jumpReleased = false;  // jump released this frame
    float jumpDir = 0.0f;       // -1.0 (left), 0.0 (neutral), +1.0 (right)
    bool splitPressed = false;  // Wiimote 1 / B on press, GC B / Y
    bool mergeHold = false;     // hold Down / GC X
    bool pausePressed = false;  // Plus / GC Start
    bool restartPressed = false;// Minus / GC Z
    bool exitPressed = false;   // Home button -> exit to Wii menu
    bool dialogueNextPressed = false;
};

void Init();
void Update(float dt = 1.0f / 60.0f);
const State& GetState();

} // namespace Input
