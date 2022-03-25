#ifndef DIGIT_H
#define DIGIT_H

#include <Arduino.h>

#include "Project_Matrix.h"

class Digit {
  
  public:
    Digit(Adafruit_GFX* d, byte value, uint16_t xo, uint16_t yo, uint16_t color);
    void Draw();
    void SetValue(byte value);
    void Morph(byte newValue, void (*callback)(void)); // callback is called on end of animation
    byte Value();
    void DrawColon();
    void SetColor(uint16_t c);
    void SetColon(bool c);
    void Animate(void);
    void AnimateMorph(void);

    void StartAnimation();
    void EndAnimation();
    
  private:
    Adafruit_GFX* _display;
    void (*_callback)(void);
    byte _value;
    byte _new_value;
    uint16_t _color;
    bool _colon;
    uint16_t xOffset;
    uint16_t yOffset;
    int _counter;
    int animSpeed = 30;

    enum {
      STATE_IDLE,
      STATE_ANIMATION
    } animation_state;

    void drawPixel(uint16_t x, uint16_t y, uint16_t c);
    void drawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c);
    void drawLine(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t c);
    void drawSeg(byte seg);
    void Morph2();
    void Morph3();
    void Morph4();
    void Morph5();
    void Morph6();
    void Morph7();
    void Morph8();
    void Morph9();
    void Morph0();
    void Morph1();
};

#endif
