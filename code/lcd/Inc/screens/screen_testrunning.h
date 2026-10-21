#ifndef SCREEN_RUNNINGTEST_H_
#define SCREEN_RUNNINGTEST_H_

#include "input_events.h"
#include "screens.h"

void screen_runningtest_on_enter (void);
void screen_runningtest_on_exit (void);
void screen_runningtest_render (void);
ScreenID screen_runningtest_handle_event (InputEvent);

#endif /* SCREEN_RUNNINGTEST_H_ */