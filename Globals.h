#pragma once
#include "ImuLogic.h"
extern Madgwick filter;
extern float pitch;

union I2Cdata_ {
  struct __attribute__((packed)){
    uint8_t left[7];
    uint8_t right[7];
    int8_t pitch;
  };
  uint8_t bytes[15];
};
extern union I2Cdata_ I2Cdata;

union vu_ {
  struct __attribute__((packed)){
    uint8_t left[7];
    uint8_t right[7];
  };
  uint8_t bytes[14];
};
extern union vu_ vu;

union vuPacket_ {
  struct __attribute__((packed)){
    uint8_t packetType = 3;
    uint8_t left[7];
    uint8_t right[7];
  };
  uint8_t bytes[15];
};

union pitchPacket_ {
  struct __attribute__((packed)){
    uint8_t packetType = 2;
    int8_t pitch;
  };
  uint8_t bytes[2];
};

union effectPacket_ {
  struct __attribute__((packed)){
    uint8_t packetType = 1;
    uint8_t effect;
  };
  uint8_t bytes[2];
};
