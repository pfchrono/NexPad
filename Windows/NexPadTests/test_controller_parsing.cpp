#include "gtest/gtest.h"
#include "CXBOXController.h"
#include <vector>
#include <cstdint>

// --- parseDualSenseUsbReport ---
// Layout (baseOffset=1): [0]=0x01, [1]=LX, [2]=LY, [3]=RX, [4]=RY,
//                        [5]=L2, [6]=R2, [7]=pad, [8]=buttons1, [9]=buttons2
// buttons1 bit5=Cross(A=0x1000). LX=0x80 -> sThumbLX=(128-128)*256=0. L2=0x40 -> bLeftTrigger=64.

TEST(DualSenseUsb, CrossButtonAndAxes) {
    std::vector<BYTE> report = {
        0x01,        // report ID
        0x80,        // LX=0x80 -> sThumbLX=(128-128)*256=0
        0x80,        // LY
        0x80,        // RX
        0x80,        // RY
        0x40,        // L2=64
        0x00,        // R2
        0x00,        // pad
        0x28,        // buttons1: nibble=0x8 (no dpad), bit5=Cross -> XINPUT_GAMEPAD_A (0x1000)
        0x00,        // buttons2
        0x00, 0x00   // pad to reach 12 bytes
    };
    XINPUT_STATE state = {};
    EXPECT_TRUE(NexPadInternal::parseDualSenseUsbReport(report, &state));
    EXPECT_EQ(state.Gamepad.wButtons, (WORD)XINPUT_GAMEPAD_A);
    EXPECT_EQ(state.Gamepad.sThumbLX, (SHORT)0);
    EXPECT_EQ(state.Gamepad.bLeftTrigger, (BYTE)64);
}

TEST(DualSenseUsb, ShortBufferReturnsFalse) {
    std::vector<BYTE> report = {0x01, 0x80, 0x80}; // only 3 bytes, min is 12
    XINPUT_STATE state = {};
    EXPECT_FALSE(NexPadInternal::parseDualSenseUsbReport(report, &state));
}

TEST(DualSenseUsb, WrongReportIdReturnsFalse) {
    std::vector<BYTE> report(12, 0x00); // report[0] != 0x01
    XINPUT_STATE state = {};
    EXPECT_FALSE(NexPadInternal::parseDualSenseUsbReport(report, &state));
}

// --- parseDualSenseBluetoothSimpleReport ---
// Layout: [0]=0x01, [1]=LX, [2]=LY, [3]=RX, [4]=RY,
//         [5]=buttons1, [6]=buttons2, [7]=pad, [8]=L2, [9]=R2
// buttons1 bit6=Circle(B=0x2000), buttons2 bit1=R1(0x0200). R2=0x80 -> 128.

TEST(DualSenseBtSimple, CircleAndR1Buttons) {
    std::vector<BYTE> report = {
        0x01,  // report ID
        0x80,  // LX
        0x80,  // LY
        0x80,  // RX
        0x80,  // RY
        0x48,  // buttons1: nibble=0x8 (no dpad), bit6=Circle -> XINPUT_GAMEPAD_B (0x2000)
        0x02,  // buttons2: bit1=R1 -> XINPUT_GAMEPAD_RIGHT_SHOULDER (0x0200)
        0x00,  // pad
        0x00,  // L2
        0x80   // R2=128
    };
    XINPUT_STATE state = {};
    EXPECT_TRUE(NexPadInternal::parseDualSenseBluetoothSimpleReport(report, &state));
    EXPECT_EQ(state.Gamepad.wButtons, (WORD)(XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_RIGHT_SHOULDER));
    EXPECT_EQ(state.Gamepad.bRightTrigger, (BYTE)128);
}

TEST(DualSenseBtSimple, ShortBufferReturnsFalse) {
    std::vector<BYTE> report = {0x01, 0x80}; // only 2 bytes, min is 10
    XINPUT_STATE state = {};
    EXPECT_FALSE(NexPadInternal::parseDualSenseBluetoothSimpleReport(report, &state));
}

// --- parseDualSenseBluetoothEnhancedReport ---
// Layout: [0]=0x31, [1]=pad, baseOffset=2 mirrors USB layout.
// buttons1 at index [8] (baseOffset+6), buttons2 at index [9] (baseOffset+7).
// Min size: 55 bytes (baseOffset+52+1).
// buttons1=0x80 bit7=Triangle -> XINPUT_GAMEPAD_Y (0x8000)
// buttons2=0x01 bit0=L1 -> XINPUT_GAMEPAD_LEFT_SHOULDER (0x0100)

TEST(DualSenseBtEnhanced, TriangleAndL1Buttons) {
    std::vector<BYTE> report(55, 0x00);
    report[0] = 0x31;  // BT enhanced report ID
    report[9] = 0x88;  // buttons1 (baseOffset+7=2+7=9): nibble=0x8 (no dpad), bit7=Triangle -> 0x8000
    report[10] = 0x01; // buttons2 (baseOffset+8=2+8=10): bit0=L1 -> 0x0100
    XINPUT_STATE state = {};
    EXPECT_TRUE(NexPadInternal::parseDualSenseBluetoothEnhancedReport(report, &state));
    EXPECT_EQ(state.Gamepad.wButtons, (WORD)(XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_LEFT_SHOULDER));
}

TEST(DualSenseBtEnhanced, ShortBufferReturnsFalse) {
    std::vector<BYTE> report(10, 0x31); // too short (min 55)
    XINPUT_STATE state = {};
    EXPECT_FALSE(NexPadInternal::parseDualSenseBluetoothEnhancedReport(report, &state));
}

TEST(DualSenseBtEnhanced, WrongReportIdReturnsFalse) {
    std::vector<BYTE> report(55, 0x00); // report[0] != 0x31
    XINPUT_STATE state = {};
    EXPECT_FALSE(NexPadInternal::parseDualSenseBluetoothEnhancedReport(report, &state));
}
