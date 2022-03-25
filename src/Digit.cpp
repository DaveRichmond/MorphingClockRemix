#include "Digit.h"

const byte sA = 0;
const byte sB = 1;
const byte sC = 2;
const byte sD = 3;
const byte sE = 4;
const byte sF = 5;
const byte sG = 6;
const int segHeight = 6;
const int segWidth = segHeight;
const uint16_t height = 31;
const uint16_t width = 63;

byte digitBits[] = {
  B11111100, // 0 ABCDEF--
  B01100000, // 1 -BC-----
  B11011010, // 2 AB-DE-G-
  B11110010, // 3 ABCD--G-
  B01100110, // 4 -BC--FG-
  B10110110, // 5 A-CD-FG-
  B10111110, // 6 A-CDEFG-
  B11100000, // 7 ABC-----
  B11111110, // 8 ABCDEFG-
  B11110110, // 9 ABCD_FG-
};

//byte sunBitmap[] {
//  B100100100,
//  B010001000,
//  B001110000,
//  B101110100,
//  B001110000,
//  B010001000,
//  B100100100
//};

uint16_t black;

Digit::Digit(Adafruit_GFX* d, byte value, uint16_t xo, uint16_t yo, uint16_t color) {
  _display = d;
  _value = value;
  xOffset = xo;
  yOffset = yo;
  _color = color;
  _colon = false;
}

byte Digit::Value() {
  return _value;
}
void Digit::drawPixel(uint16_t x, uint16_t y, uint16_t c)
{
  _display->drawPixel(xOffset + x, height - (y + yOffset), c);
}

void Digit::drawLine(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t c)
{
  _display->drawLine(xOffset + x, height - (y + yOffset), xOffset + x2, height - (y2 + yOffset), c);
}

void Digit::drawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c)
{
  _display->fillRect(xOffset + x, height - (y + yOffset), w,h, c);
}

void Digit::SetColor(uint16_t c)
{
  //Serial.println("Colour set to: " + String(c));
  _color = c;
}
void Digit::SetColon(bool c){
  _colon = c;
}

void Digit::DrawColon()
{
  // Colon is drawn to the left of this digit
  drawFillRect(-3, segHeight-1, 2,2, _color);
  drawFillRect(-3, segHeight+1+3, 2,2, _color);
}

void Digit::drawSeg(byte seg)
{
  switch (seg) {
    case sA: drawLine(1, segHeight * 2 + 2, segWidth, segHeight * 2 + 2, _color); break;
    case sB: drawLine(segWidth + 1, segHeight * 2 + 1, segWidth + 1, segHeight + 2, _color); break;
    case sC: drawLine(segWidth + 1, 1, segWidth + 1, segHeight, _color); break;
    case sD: drawLine(1, 0, segWidth, 0, _color); break;
    case sE: drawLine(0, 1, 0, segHeight, _color); break;
    case sF: drawLine(0, segHeight * 2 + 1, 0, segHeight + 2, _color); break;
    case sG: drawLine(1, segHeight + 1, segWidth, segHeight + 1, _color); break;
  }
}

void Digit::Draw() {
  byte pattern = digitBits[_value];
  if (bitRead(pattern, 7)) drawSeg(sA);
  if (bitRead(pattern, 6)) drawSeg(sB);
  if (bitRead(pattern, 5)) drawSeg(sC);
  if (bitRead(pattern, 4)) drawSeg(sD);
  if (bitRead(pattern, 3)) drawSeg(sE);
  if (bitRead(pattern, 2)) drawSeg(sF);
  if (bitRead(pattern, 1)) drawSeg(sG);
}

void Digit::SetValue(byte value){
  _value = value;
}

void Digit::StartAnimation(){
  _counter = 0;
  animation_state = STATE_ANIMATION;
  //Serial.println("Beginning Animation");
}
void Digit::EndAnimation(){
  Serial.println("Ending animation");
  if(_callback != nullptr){
    _callback();
    _callback = nullptr; // cleanup callback
  }
  _value = _new_value;
  animation_state = STATE_IDLE;
}

void Digit::Animate(){
  // FIXME

  switch(animation_state){
    case STATE_ANIMATION:
      AnimateMorph();
      break;
    default:
      Draw();
  }

  if(_colon){
    DrawColon();
  }
  _counter++;
}

void Digit::Morph2() {
  // TWO
  if (_counter < segWidth) {
    drawPixel(segWidth - _counter, segHeight * 2 + 2, _color);
    drawPixel(segWidth - _counter, segHeight + 1, _color);
    drawPixel(segWidth - _counter, 0, _color);
  }

  drawLine(segWidth + 1 - _counter, 1, segWidth + 1 - _counter, segHeight, black);
  drawLine(segWidth - _counter, 1, segWidth - _counter, segHeight, _color);
  if(_counter >= segWidth){
    EndAnimation();
  }
}

void Digit::Morph3() {
  // THREE
  drawLine(0 + _counter, 1, 0 + _counter, segHeight, black);
  drawLine(1 + _counter, 1, 1 + _counter, segHeight, _color);
  if(_counter >= segWidth){
    EndAnimation();
  }
}

void Digit::Morph4() {
  // FOUR
  drawPixel(segWidth - _counter, segHeight * 2 + 2, black); // Erase A
  drawPixel(0, segHeight * 2 + 1 - _counter, _color); // Draw as F
  drawPixel(1 + _counter, 0, black); // Erase D
  if(_counter >= segWidth){
    EndAnimation();
  }
}

void Digit::Morph5() {
  // FIVE
  drawPixel(segWidth + 1, segHeight + 2 + _counter, black); // Erase B
  drawPixel(segWidth - _counter, segHeight * 2 + 2, _color); // Draw as A
  drawPixel(segWidth - _counter, 0, _color); // Draw D
  if(_counter >= segWidth){
    EndAnimation();
  }
}

void Digit::Morph6() {
  // SIX
  // Move C right to left
  drawLine(segWidth - _counter, 1, segWidth - _counter, segHeight, _color);
  if (_counter > 0) drawLine(segWidth - _counter + 1, 1, segWidth - _counter + 1, segHeight, black);
  
  if(_counter >= segWidth){
    EndAnimation();
  } 
}

void Digit::Morph7() {
  // SEVEN
  // Move E left to right
  drawLine(0 + _counter - 1, 1, 0 + _counter - 1, segHeight, black);
  drawLine(0 + _counter, 1, 0 + _counter, segHeight, _color);

  // Move F left to right
  drawLine(0 + _counter - 1, segHeight * 2 + 1, 0 + _counter - 1, segHeight + 2, black);
  drawLine(0 + _counter, segHeight * 2 + 1, 0 + _counter, segHeight + 2, _color);

  // Erase D and G gradually
  drawPixel(1 + _counter, 0, black); // D
  drawPixel(1 + _counter, segHeight + 1, black); // G
  if(_counter >= (segWidth + 1)){
    EndAnimation();
  }
}

void Digit::Morph8() {
  // EIGHT
  // Move B right to left
  drawLine(segWidth - _counter, segHeight * 2 + 1, segWidth - _counter, segHeight + 2, _color);
  if (_counter > 0) drawLine(segWidth - _counter + 1, segHeight * 2 + 1, segWidth - _counter + 1, segHeight + 2, black);

  // Move C right to left
  drawLine(segWidth - _counter, 1, segWidth - _counter, segHeight, _color);
  if (_counter > 0) drawLine(segWidth - _counter + 1, 1, segWidth - _counter + 1, segHeight, black);

  // Gradually draw D and G
  if (_counter < segWidth) {
    drawPixel(segWidth - _counter, 0, _color); // D
    drawPixel(segWidth - _counter, segHeight + 1, _color); // G
  }
  if(_counter >= segWidth){
    EndAnimation();
  }
}

void Digit::Morph9() {
  // NINE
  // Move E left to right
  drawLine(0 + _counter - 1, 1, 0 + _counter - 1, segHeight, black);
  drawLine(0 + _counter, 1, 0 + _counter, segHeight, _color);
  if(_counter >= (segWidth + 1)){
    EndAnimation();
  }
}

void Digit::Morph0() {
  // ZERO
  if (_value==1) { // If 1 to 0, slide B to F and E to C  
    // slide B to F 
    drawLine(segWidth - _counter, segHeight * 2+1 , segWidth - _counter, segHeight + 2, _color);
    if (_counter > 0) drawLine(segWidth - _counter + 1, segHeight * 2+1, segWidth - _counter + 1, segHeight + 2, black);

    // slide E to C
    drawLine(segWidth - _counter, 1, segWidth - _counter, segHeight, _color);
    if (_counter > 0) drawLine(segWidth - _counter + 1, 1, segWidth - _counter + 1, segHeight, black);

    if (_counter<segWidth) drawPixel(segWidth - _counter, segHeight * 2 + 2 , _color); // Draw A
    if (_counter<segWidth) drawPixel(segWidth - _counter, 0, _color); // Draw D
  }
  
  if (_value==2) { // If 2 to 0, slide B to F and Flow G to C
    // slide B to F 
    drawLine(segWidth - _counter, segHeight * 2+1 , segWidth - _counter, segHeight + 2, _color);
    if (_counter > 0) drawLine(segWidth - _counter + 1, segHeight * 2+1, segWidth - _counter + 1, segHeight + 2, black);
  
    drawPixel(1+_counter, segHeight + _counter, black); // Erase G left to right
    if (_counter<segWidth) drawPixel(segWidth + 1, segHeight + 1- _counter, _color);// Draw C
  }

  if (_value==3) { // B to F, C to E
    // slide B to F 
    drawLine(segWidth - _counter, segHeight * 2+1 , segWidth - _counter, segHeight + 2, _color);
    if (_counter > 0) drawLine(segWidth - _counter + 1, segHeight * 2+1, segWidth - _counter + 1, segHeight + 2, black);
    
    // Move C to E
    drawLine(segWidth - _counter, 1, segWidth - _counter, segHeight, _color);
    if (_counter > 0) drawLine(segWidth - _counter + 1, 1, segWidth - _counter + 1, segHeight, black);

    // Erase G from right to left
    drawPixel(segWidth - _counter, segHeight + _counter, black); // G
  }
  
  if (_value==5) { // If 5 to 0, we also need to slide F to B
    if (_counter<segWidth) {
      if (_counter>0) drawLine(1 + _counter, segHeight * 2 + 1, 1 + _counter, segHeight + 2, black);
      drawLine(2 + _counter, segHeight * 2 + 1, 2 + _counter, segHeight + 2, _color);
    }
  }
  
  if (_value==5 || _value==9) { // If 9 or 5 to 0, Flow G into E
    if (_counter<segWidth) drawPixel(segWidth - _counter, segHeight + 1, black);
    if (_counter<segWidth) drawPixel(0, segHeight - _counter, _color);
  }
  if(_counter >= segWidth){
    EndAnimation();
  }
}

void Digit::Morph1() {
  // Zero or two to One
  // Move E left to right
  drawLine(0 + _counter - 1, 1, 0 + _counter - 1, segHeight, black);
  drawLine(0 + _counter, 1, 0 + _counter, segHeight, _color);

  // Move F left to right
  drawLine(0 + _counter - 1, segHeight * 2 + 1, 0 + _counter - 1, segHeight + 2, black);
  drawLine(0 + _counter, segHeight * 2 + 1, 0 + _counter, segHeight + 2, _color);

  // Gradually Erase A, G, D
  drawPixel(1 + _counter, segHeight * 2 + 2, black); // A
  drawPixel(1 +_counter, 0, black); // D
  drawPixel(1 + _counter, segHeight + 1, black); // G

  if(_counter >= (segWidth + 1)){
    EndAnimation();
  }
}

void Digit::Morph(byte newValue, void (*callback)(void)){
  if(_value != newValue){
    _new_value = newValue;
    _callback = callback;
    StartAnimation();
  }
}

void Digit::AnimateMorph() {
  //Serial.println("Morph: " + String(_new_value) + "(" + String(_counter) + ")");
  switch (_new_value) {
    case 2: Morph2(); break;
    case 3: Morph3(); break;
    case 4: Morph4(); break;
    case 5: Morph5(); break;
    case 6: Morph6(); break;
    case 7: Morph7(); break;
    case 8: Morph8(); break;
    case 9: Morph9(); break;
    case 0: Morph0(); break;
    case 1: Morph1(); break;
  }
}

