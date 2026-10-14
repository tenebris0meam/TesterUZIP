#ifndef SCREEN_H_
#define SCREEN_H_

#include "input_events.h"
#include "strbuf.h"
#include "params.h"

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
    ScreenSettingsModes,
    ScreenSettingsSeries,
    ScreenSettingsGroups,
    ScreenSettingsDevices,
    ScreenHardwareFail,
    ScreenGeneratorSetup,
    ScreenGenerator,
    ScreenCount 
} ScreenID;

typedef struct {
    void (*on_enter) (void);
    void (*on_exit) (void);
    void (*render) (void);
    ScreenID (*handle_event) (InputEvent);
} Screen_TypeDef;

/** @brief Какой список имён показывает экран настроек. */
typedef enum {
    NameList_Series,
    NameList_Groups,
    NameList_Devices
} NameListKind;

extern const Screen_TypeDef screens[ScreenCount];
extern char *linebuffer;
extern uint8_t linebuffer_size;

extern uint8_t workspace_start_x;
extern uint8_t workspace_start_y;
extern uint8_t workspace_width_in_px;
extern uint8_t workspace_height_in_px;
extern uint8_t workspace_width_in_chars;
extern uint8_t workspace_height_in_chars;

void screen_render_name_list (const char *first_word, 
                                const char *second_word, 
                                NameListKind kind);
const MyString *screen_name_at (NameListKind kind, uint8_t idx);
uint8_t screen_name_count (NameListKind kind);
void screen_draw_title (StrBuf_TypeDef *sb, 
                        const char *first_word, 
                        const char *second_word);
void screen_append_pins (StrBuf_TypeDef *sb, 
                        const Params_TestSequence_TypeDef *sequence);
void screen_get_center_from_string (uint8_t *center_x_px, 
                                    uint8_t str_length);
void screen_append_device_name (StrBuf_TypeDef *sb);                                    
void screen_append_mystring (StrBuf_TypeDef *sb, const MyString *name);
const char *screen_interface_name (Param_Connector interface_id);

#endif /* SCREEN_H_ */
