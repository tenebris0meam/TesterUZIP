#include "fonts.h"
#include "CGXYZLCD6x8.h"
#include "LCDFont7x14.h"

const Font_TypeDef cgxyz = { 
    .width_px = 6, 
    .height_px = 8, 
    .font_cyr = CGXYZ_LCD6x8_Cyrilic, 
    .font_lat = CGXYZ_LCD6x8_Latin 
};

const Font_TypeDef jetbrains = {
    .width_px = 7,
    .height_px = 14, 
    .font_cyr = JetBrains_Mono7x13_Cyrilic, 
    .font_lat = JetBrains_Mono7x13_Latin
};