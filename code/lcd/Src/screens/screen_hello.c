#include "screens/screen_hello.h"
#include "tachyon_logo.h"
#include "framebuffer.h"

void screen_hello_on_enter (void) {
    return;
}

void screen_hello_render (void) {
    fb_draw_bitmap((FB_WIDTH - LOGO_WIDTH) / 2, 
                   (FB_HEIGHT - LOGO_HEIGHT) / 2, 
                   LOGO_WIDTH, 
                   LOGO_HEIGHT, 
                   logo);
    return;
}

ScreenID screen_hello_handle_event(InputEvent event) {
    switch (event) {
        case InputEvent_HelloScreenTimeout: return ScreenMain;
        default: return ScreenHello;
    }
}

void screen_hello_on_exit (void) {
    return;
}