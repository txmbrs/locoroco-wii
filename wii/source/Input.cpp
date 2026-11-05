#include "Input.hpp"
#include "MotionTilt.hpp"
#include <wiiuse/wpad.h>
#include <ogc/pad.h>
#include <algorithm>
#include <cmath>

namespace Input {

static State s_state;
static bool s_lastJump = false;
static bool s_wasChargingBoth = false;
static MotionTilt s_motion;

void Init() {
    s_motion=MotionTilt{};
    s_state=State{};
    s_lastJump=s_wasChargingBoth=false;
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
    PAD_Init();
}

void Update(float dt) {
    WPAD_ScanPads();
    PAD_ScanPads();

    u32 wDown = WPAD_ButtonsDown(0);
    u32 wHeld = WPAD_ButtonsHeld(0);

    u16 gcDown = PAD_ButtonsDown(0);
    u16 gcHeld = PAD_ButtonsHeld(0);
    s8  gcStickX = PAD_StickX(0);

    s_state = State();
    s_state.dialogueNextPressed = (wDown & (WPAD_BUTTON_A | WPAD_BUTTON_2)) || (gcDown & PAD_BUTTON_A);

    // 1. Tilt directions (Wiimote sideways / D-Pad / GC Triggers / GC D-Pad / Wiimote 1 & 2)
    bool leftTilt = (wHeld & WPAD_BUTTON_LEFT) || (wHeld & WPAD_BUTTON_UP) || (gcHeld & PAD_TRIGGER_L) || (gcHeld & PAD_BUTTON_LEFT);
    bool rightTilt = (wHeld & WPAD_BUTTON_RIGHT) || (gcHeld & PAD_TRIGGER_R) || (gcHeld & PAD_BUTTON_RIGHT);

    // Also support Wiimote sideways 1+2 as L+R
    bool btn1Held = (wHeld & WPAD_BUTTON_1) != 0;
    bool btn2Held = (wHeld & WPAD_BUTTON_2) != 0;
    bool btnAHeld = (wHeld & WPAD_BUTTON_A) || (gcHeld & PAD_BUTTON_A);

    bool bothLRHeld = (leftTilt && rightTilt) || (btn1Held && btn2Held);
    bool anyJumpHeld = bothLRHeld || btn2Held || btnAHeld;

    s_state.jumpHold = anyJumpHeld;

    if (bothLRHeld) {
        s_wasChargingBoth = true;
    }

    if (!anyJumpHeld && s_lastJump) {
        s_state.jumpReleased = true;
        // Direction based on what was held or stick
        if (s_wasChargingBoth) {
            if (rightTilt && !leftTilt) {
                s_state.jumpDir = 1.0f; // Released L while holding R -> Jump RIGHT!
            } else if (leftTilt && !rightTilt) {
                s_state.jumpDir = -1.0f; // Released R while holding L -> Jump LEFT!
            } else {
                s_state.jumpDir = 0.0f; // Released both -> Jump straight UP!
            }
        } else {
            // Standard jump button release: jump in direction of tilt
            if (rightTilt || gcStickX > 25) s_state.jumpDir = 1.0f;
            else if (leftTilt || gcStickX < -25) s_state.jumpDir = -1.0f;
            else s_state.jumpDir = 0.0f;
        }
        s_wasChargingBoth = false;
    } else if (s_wasChargingBoth && !bothLRHeld) {
        // One of the two triggers was released while charging!
        s_state.jumpReleased = true;
        if (rightTilt && !leftTilt) {
            s_state.jumpDir = 1.0f; // Released L while holding R -> Jump RIGHT!
        } else if (leftTilt && !rightTilt) {
            s_state.jumpDir = -1.0f; // Released R while holding L -> Jump LEFT!
        } else {
            s_state.jumpDir = 0.0f;
        }
        s_wasChargingBoth = false;
        s_state.jumpHold = false;
    }

    s_lastJump = anyJumpHeld;

    float dpadTilt = 0.0f;
    if (leftTilt && !rightTilt) dpadTilt -= 1.0f;
    if (rightTilt && !leftTilt) dpadTilt += 1.0f;

    // Analog stick tilt
    float gcTilt = 0.0f;
    if (std::abs(gcStickX) > 15) {
        gcTilt = static_cast<float>(gcStickX) / 70.0f;
    }

    // Wiimote motion tilt (roll angle when held horizontally or pointed)
    orient_t orient{};
    WPAD_Orientation(0, &orient);
    float rollTilt = s_motion.Update(orient.roll,dt);

    // Combined tilt (if charging L+R together, level out to 0 like PSP)
    if (bothLRHeld) {
        s_state.tilt = 0.0f;
    } else {
        s_state.tilt = dpadTilt!=0.0f ? dpadTilt : gcTilt!=0.0f ? gcTilt : rollTilt;
        s_state.tilt = std::clamp(s_state.tilt, -1.0f, 1.0f);
    }
    if (s_state.jumpReleased && s_state.jumpDir==0.0f && std::abs(s_state.tilt)>0.15f)
        s_state.jumpDir=s_state.tilt>0 ? 1.0f : -1.0f;

    // 3. Split (Button 1 / B / GC B / GC Y)
    s_state.splitPressed = ((wDown & WPAD_BUTTON_1) && !btn2Held) || (wDown & WPAD_BUTTON_B) || (gcDown & PAD_BUTTON_B) || (gcDown & PAD_BUTTON_Y);

    // 4. Merge (Hold D-Pad Down / GC X / GC Down)
    s_state.mergeHold = (wHeld & WPAD_BUTTON_DOWN) || (gcHeld & PAD_BUTTON_X) || (gcHeld & PAD_BUTTON_DOWN);

    // 5. Pause & Restart & Exit
    s_state.pausePressed = (wDown & WPAD_BUTTON_PLUS) || (gcDown & PAD_BUTTON_START);
    s_state.restartPressed = (wDown & WPAD_BUTTON_MINUS) || (gcDown & PAD_TRIGGER_Z);
    s_state.exitPressed = (wDown & WPAD_BUTTON_HOME);
}

const State& GetState() {
    return s_state;
}

} // namespace Input
