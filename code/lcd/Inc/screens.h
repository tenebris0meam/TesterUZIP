#ifndef SCREEN_H_
#define SCREEN_H_

#include "input_events.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ScreenHello,
    ScreenMain, 
    ScreenTesting,
    ScreenRunningTest,
    ScreenResult,
    ScreenCheckFailed,
    ScreenSettings,
    ScreenSettingsInterface,
    ScreenSettingsSeries,
    ScreenSettingsGroups,
    ScreenSettingsDevices,
    ScreenHardwareFail,
    ScreenCount 
} ScreenID;

typedef struct {
    void (*on_enter) (void);
    void (*on_exit) (void);
    void (*render) (void);
    ScreenID (*handle_event) (InputEvent);
} Screen_TypeDef;

extern const Screen_TypeDef screens[ScreenCount];

void screen_hello_on_enter (void);
void screen_main_on_enter (void);
void screen_testing_on_enter (void);
void screen_runningtest_on_enter (void);
void screen_result_on_enter (void);
void screen_checkfailed_on_enter (void);
void screen_settings_on_enter (void);
void screen_settingsinterface_on_enter (void);
void screen_settingsseries_on_enter (void);
void screen_settingsgroups_on_enter (void);
void screen_settingsdevices_on_enter (void);
void screen_hardwarefail_on_enter (void);

void screen_hello_on_exit (void);
void screen_main_on_exit (void);
void screen_testing_on_exit (void);
void screen_runningtest_on_exit (void);
void screen_result_on_exit (void);
void screen_checkfailed_on_exit (void);
void screen_settings_on_exit (void);
void screen_settingsinterface_on_exit (void);
void screen_settingsseries_on_exit (void);
void screen_settingsgroups_on_exit (void);
void screen_settingsdevices_on_exit (void);
void screen_hardwarefail_on_exit (void);

void screen_hello_render (void);
void screen_main_render (void);
void screen_testing_render (void);
void screen_runningtest_render (void);
void screen_result_render (void);
void screen_checkfailed_render (void);
void screen_settings_render (void);
void screen_settingsinterface_render (void);
void screen_settingsseries_render (void);
void screen_settingsgroups_render (void);
void screen_settingsdevices_render (void);
void screen_hardwarefail_render (void);

ScreenID screen_hello_handle_event (InputEvent);
ScreenID screen_main_handle_event (InputEvent);
ScreenID screen_testing_handle_event (InputEvent);
ScreenID screen_runningtest_handle_event (InputEvent);
ScreenID screen_result_handle_event (InputEvent);
ScreenID screen_checkfailed_handle_event (InputEvent);
ScreenID screen_settings_handle_event (InputEvent);
ScreenID screen_settingsinterface_handle_event (InputEvent);
ScreenID screen_settingsseries_handle_event (InputEvent);
ScreenID screen_settingsgroups_handle_event (InputEvent);
ScreenID screen_settingsdevices_handle_event (InputEvent);
ScreenID screen_hardwarefail_handle_event (InputEvent);

#endif /* SCREEN_H_ */
