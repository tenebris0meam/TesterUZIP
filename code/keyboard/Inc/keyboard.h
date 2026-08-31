#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <stdbool.h>

#if defined DEBUG

#define BUTTON1_LOWTH_uV (0UL)
#define BUTTON1_HIGHTH_uV (100000UL)



// #define BUTTON2_LOWTH_uV (1149500UL)
// #define BUTTON2_HIGHTH_uV (1270500UL)

// #define BUTTON3_LOWTH_uV (1719500UL)
// #define BUTTON3_HIGHTH_uV (1900500UL)



#define BUTTON2_LOWTH_uV (741870UL)
#define BUTTON2_HIGHTH_uV (819962UL)

#define BUTTON3_LOWTH_uV (1402956UL)
#define BUTTON3_HIGHTH_uV (1550636UL)



#define BUTTON4_LOWTH_uV (2228930UL)
#define BUTTON4_HIGHTH_uV (2463555UL)

#define BUTTON5_LOWTH_uV (2764432UL)
#define BUTTON5_HIGHTH_uV (3055426UL)

#else /* !defined DEBUG */

#define BUTTON1_LOWTH_uV (0UL)
#define BUTTON1_HIGHTH_uV (100000UL)

#define BUTTON2_LOWTH_uV (741870UL)
#define BUTTON2_HIGHTH_uV (819962UL)

#define BUTTON3_LOWTH_uV (1402956UL)
#define BUTTON3_HIGHTH_uV (1550636UL)

#define BUTTON4_LOWTH_uV (2228930UL)
#define BUTTON4_HIGHTH_uV (2463555UL)

#define BUTTON5_LOWTH_uV (2764432UL)
#define BUTTON5_HIGHTH_uV (3055426UL)

#endif /* defined DEBUG */

typedef enum {
    Previous,
    Cancel,
    Next,
    Enter,
#if defined DEBUG
    Beep
#endif
} Keyboard_ButtonNames;

typedef enum {
    NoActive,
    PreviousActive,
    CancelActive,
    NextActive,
    EnterActive,
#if defined DEBUG
    BeepActive
#endif
} Keyboard_ActiveButton;

void keyboard_init (void);
void keyboard_routine (void);
Keyboard_ActiveButton keyboard_get_active_button (void);
bool keyboard_is_button_pressed (void);

#endif /* KEYBOARD_H_ */