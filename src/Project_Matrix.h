#ifndef _PROJECT_MATRIX_H
#define _PROJECT_MATRIX_H

#include <stdint.h>

#define USE_ADAFRUIT_GFX_LAYERS
#include <MatrixHardware_ESP32_HUB75Custom.h>
#include <SmartMatrix.h>

#define COLOR_DEPTH 24

const uint16_t kMatrixWidth = 64;
const uint16_t kMatrixHeight = 32;
const uint8_t kRefreshDepth = 36;
const uint8_t kDmaBufferRows = 4;
const uint8_t kPanelType = SM_PANELTYPE_HUB75_32ROW_MOD16SCAN;
const uint32_t kMatrixOptions = (SM_HUB75_OPTIONS_NONE);
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);

const int defaultBrightness = (50*255)/100;        // half (50%) brightness
const int defaultScrollOffset = 6;
const rgb24 defaultBackgroundColor = {0x40, 0, 0};

//typedef RGB_TYPE(COLOR_DEPTH) SM_RGB; 
//using SMLayerBackground_t = SMLayerBackground<RGB_TYPE(COLOR_DEPTH), kBackgroundLayerOptions>;

#endif