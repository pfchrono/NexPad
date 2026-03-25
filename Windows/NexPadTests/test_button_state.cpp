#include "gtest/gtest.h"
#include "NexPad.h"
#include "CXBOXController.h"
#include <windows.h>  // for Sleep

// Fixture: creates a disconnected CXBOXController + NexPad.
// setXboxClickState() does NOT call GetState() or SendInput — safe in unit tests.
class ButtonStateTest : public ::testing::Test {
protected:
    CXBOXController controller{1};
    NexPad nexPad{&controller};
};

TEST_F(ButtonStateTest, RisingEdge_TrueOnFirstPress) {
    nexPad.setTestButtonState(XINPUT_GAMEPAD_A);
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);
    EXPECT_TRUE(nexPad.isButtonDown(XINPUT_GAMEPAD_A));
    EXPECT_FALSE(nexPad.isButtonUp(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, RisingEdge_FalseOnSecondTick) {
    nexPad.setTestButtonState(XINPUT_GAMEPAD_A);
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // tick 1: press
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // tick 2: still held
    EXPECT_FALSE(nexPad.isButtonDown(XINPUT_GAMEPAD_A));  // no longer rising
    EXPECT_FALSE(nexPad.isButtonUp(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, FallingEdge_TrueOnRelease) {
    nexPad.setTestButtonState(XINPUT_GAMEPAD_A);
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // press (tick 1)
    nexPad.setTestButtonState(0);
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // release (tick 2: button state is 0 but we check XINPUT_GAMEPAD_A)
    EXPECT_TRUE(nexPad.isButtonUp(XINPUT_GAMEPAD_A));
    EXPECT_FALSE(nexPad.isButtonDown(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, FallingEdge_FalseOnSecondTickAfterRelease) {
    nexPad.setTestButtonState(XINPUT_GAMEPAD_A);
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // press
    nexPad.setTestButtonState(0);
    nexPad.setXboxClickState(0);                  // release
    nexPad.setXboxClickState(0);                  // still released
    EXPECT_FALSE(nexPad.isButtonUp(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, LongHold_TrueAfter200ms) {
    // NexPad runs at ~150 Hz with SLEEP_AMOUNT = 1000/150 ≈ 6.67ms per tick.
    // Long press threshold = 200 ms.
    // Condition: _xboxClickDownLength[STATE] * SLEEP_AMOUNT > 200
    // With SLEEP_AMOUNT = 6: need _xboxClickDownLength > 33.33, so at least 34 increments.
    nexPad.setTestButtonState(XINPUT_GAMEPAD_A);
    for (int i = 0; i < 34; ++i) {
        nexPad.setXboxClickState(XINPUT_GAMEPAD_A);
    }
    EXPECT_TRUE(nexPad.isButtonDownLong(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, LongHold_FalseBeforeThreshold) {
    nexPad.setTestButtonState(XINPUT_GAMEPAD_A);
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // press
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // held immediately
    EXPECT_FALSE(nexPad.isButtonDownLong(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, Combo_BothBitsRequired) {
    const DWORD combo = XINPUT_GAMEPAD_START | XINPUT_GAMEPAD_BACK; // 0x0030
    nexPad.setTestButtonState((WORD)combo);
    nexPad.setXboxClickState(combo);
    EXPECT_TRUE(nexPad.isButtonDown(combo));
    EXPECT_FALSE(nexPad.isButtonDown(XINPUT_GAMEPAD_START));  // individual keys not tracked separately
    EXPECT_FALSE(nexPad.isButtonDown(XINPUT_GAMEPAD_BACK));
}
