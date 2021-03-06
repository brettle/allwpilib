/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "DSCommPacket.h"
#include "gtest/gtest.h"

class DSCommPacketTest : public ::testing::Test {
 public:
  DSCommPacketTest() {}

  void SendJoysticks() { commPacket.SendJoysticks(); }

  halsim::DSCommJoystickPacket& ReadJoystickTag(wpi::ArrayRef<uint8_t> data,
                                                int index) {
    commPacket.ReadJoystickTag(data, index);
    return commPacket.m_joystick_packets[index];
  }

  halsim::DSCommJoystickPacket& ReadDescriptorTag(wpi::ArrayRef<uint8_t> data) {
    commPacket.ReadJoystickDescriptionTag(data);
    return commPacket.m_joystick_packets[data[3]];
  }

  HAL_MatchInfo& ReadNewMatchInfoTag(wpi::ArrayRef<uint8_t> data) {
    commPacket.ReadNewMatchInfoTag(data);
    return commPacket.matchInfo;
  }

  HAL_MatchInfo& ReadGameSpecificTag(wpi::ArrayRef<uint8_t> data) {
    commPacket.ReadGameSpecificMessageTag(data);
    return commPacket.matchInfo;
  }

 protected:
  halsim::DSCommPacket commPacket;
};

TEST_F(DSCommPacketTest, EmptyJoystickTag) {
  for (int i = 0; i < HAL_kMaxJoysticks; i++) {
    uint8_t arr[2];
    auto& data = ReadJoystickTag(arr, 0);
    ASSERT_EQ(data.axes.count, 0);
    ASSERT_EQ(data.povs.count, 0);
    ASSERT_EQ(data.buttons.count, 0);
  }
}

TEST_F(DSCommPacketTest, BlankJoystickTag) {
  for (int i = 0; i < HAL_kMaxJoysticks; i++) {
    uint8_t arr[5];
    arr[0] = 4;
    arr[1] = 2;
    arr[2] = 0;
    arr[3] = 0;
    arr[4] = 0;
    auto& data = ReadJoystickTag(arr, 0);
    ASSERT_EQ(data.axes.count, 0);
    ASSERT_EQ(data.povs.count, 0);
    ASSERT_EQ(data.buttons.count, 0);
  }
}

TEST_F(DSCommPacketTest, MainJoystickTag) {
  for (int i = 0; i < HAL_kMaxJoysticks; i++) {
    // 5 for base, 4 joystick, 12 buttons (2 bytes) 3 povs
    uint8_t arr[5 + 4 + 2 + 6] = {// Size, Tag
                                  16, 12,
                                  // Axes
                                  4, 0x9C, 0xCE, 0, 75,
                                  // Buttons
                                  12, 0xFF, 0x0F,
                                  // POVs
                                  3, 0, 50, 0, 100, 0x0F, 0x00};

    auto& data = ReadJoystickTag(arr, 0);
    ASSERT_EQ(data.axes.count, 4);
    ASSERT_EQ(data.povs.count, 3);
    ASSERT_EQ(data.buttons.count, 12);
  }
}

TEST_F(DSCommPacketTest, DescriptorTag) {
  for (int i = 0; i < HAL_kMaxJoysticks; i++) {
    uint8_t arr[] = {// Size (2), tag
                     0, 0, 7,
                     // Joystick index, Is Xbox, Type
                     static_cast<uint8_t>(i), 1, 0,
                     // NameLen, Name (Not null terminated)
                     11, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd',
                     // Axes count, Axes types
                     4, 1, 2, 3, 4,
                     // Button count, pov count,
                     12, 3};
    arr[1] = sizeof(arr) - 2;
    auto& data = ReadDescriptorTag(arr);
    ASSERT_EQ(data.descriptor.isXbox, 1);
    ASSERT_EQ(data.descriptor.type, 0);
    ASSERT_STREQ(data.descriptor.name, "Hello World");
    ASSERT_EQ(data.descriptor.axisCount, 4);
    for (int i = 0; i < 4; i++) {
      ASSERT_EQ(data.descriptor.axisTypes[i], i + 1);
    }
    ASSERT_EQ(data.descriptor.buttonCount, 12);
    ASSERT_EQ(data.descriptor.povCount, 3);
  }
}

TEST_F(DSCommPacketTest, MatchInfoTag) {
  uint8_t arr[]{// Size (2), tag
                0, 0, 8,
                // Event Name Len, Event Name
                4, 'W', 'C', 'B', 'C',
                // Match type, Match num (2), replay num
                2, 0, 18, 1};
  arr[1] = sizeof(arr) - 2;
  auto& matchInfo = ReadNewMatchInfoTag(arr);
  ASSERT_STREQ(matchInfo.eventName, "WCBC");
  ASSERT_EQ(matchInfo.matchType, HAL_MatchType::HAL_kMatchType_qualification);
  ASSERT_EQ(matchInfo.matchNumber, 18);
  ASSERT_EQ(matchInfo.replayNumber, 1);
}

TEST_F(DSCommPacketTest, GameDataTag) {
  uint8_t arr[]{
      // Size (2), tag
      0,
      0,
      17,
      // Match data (length is taglength - 1)
      'W',
      'C',
      'B',
      'C',
  };
  arr[1] = sizeof(arr) - 2;
  auto& matchInfo = ReadGameSpecificTag(arr);
  ASSERT_EQ(matchInfo.gameSpecificMessageSize, 4);
  ASSERT_EQ(matchInfo.gameSpecificMessage[0], 'W');
  ASSERT_EQ(matchInfo.gameSpecificMessage[1], 'C');
  ASSERT_EQ(matchInfo.gameSpecificMessage[2], 'B');
  ASSERT_EQ(matchInfo.gameSpecificMessage[3], 'C');
}
