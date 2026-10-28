#include "screens/screen_testdata.h"
#include "calc.h"
#include "fonts.h"
#include "hv_feedback.h"
#include "input_events.h"
#include "params.h"
#include "resolver.h"
#include "screens.h"
#include "strbuf.h"
#include "strings_def.h"
#include "framebuffer.h"
#include "model.h"
#include <stdint.h>

static uint8_t choosed_idx = 0;

uint8_t screen_testdata_get_choosed_idx (void) {
    return choosed_idx;
}

void screen_testdata_set_choosed_idx (const uint8_t idx) {
    choosed_idx = idx;
    return;
}

void screen_testdata_on_enter (void) {
    return;
}

void screen_testdata_on_exit (void) {
    return;
}

void screen_testdata_render (void) {
    uint8_t cur_x = workspace_start_x;
    uint8_t cur_y = workspace_start_y;
    StrBuf_TypeDef sb;
    strbuf_init(&sb, linebuffer, linebuffer_size);

    const Params_Device_TypeDef *dev = test_runtime.current_device;
    ResolveType type_choosed_test;
    if (choosed_idx < dev->num_seqs_type0) {
        type_choosed_test = TransientVoltageSupressor;
    } else if (choosed_idx < dev->num_seqs_type0 + dev->num_seqs_type1) {
        type_choosed_test = Varistor;
    } else if (choosed_idx < dev->num_seqs_type0 + dev->num_seqs_type1 + dev->num_seqs_type2) {
        type_choosed_test = GasDischargeTube;
    } else {
        type_choosed_test = UINT8_MAX;
    }
    const Params_TestSequence_TypeDef *seq = model_get_sequence_at(choosed_idx, type_choosed_test);
    const Params_TestParam_TypeDef *testparam = model_get_testparams_at(choosed_idx, type_choosed_test);
    const CalcMetrics_TypeDef *choosed_metrics = &test_results[choosed_idx].res;

    strbuf_str(&sb, TO_TEST_CYRILIC);
    strbuf_char(&sb, ' ');
    strbuf_u32(&sb, choosed_idx + 1, 1);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    strbuf_clear(&sb);
    cur_y += cgxyz.height_px;

    strbuf_str(&sb, PROBERED_CYRILIC);
    strbuf_str(&sb, ": ");
    strbuf_u32(&sb, seq->input_pin + 1, 1);
    strbuf_str(&sb, "   ");
    strbuf_str(&sb, PROBEBLACK_CYRILIC);
    strbuf_str(&sb, ": ");
    strbuf_u32(&sb, seq->output_pin + 1, 1);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    strbuf_clear(&sb);
    cur_y += cgxyz.height_px;

    strbuf_str(&sb, TYPE_CYRILIC);
    strbuf_str(&sb, ": ");
    switch (type_choosed_test) {
        case TransientVoltageSupressor: strbuf_str(&sb, DIODE_CYRILIC); break;
        case Varistor: strbuf_str(&sb, VARISTOR_CYRILIC); break;
        case GasDischargeTube: strbuf_str(&sb, DISCHARGER_CYRILIC); break;
        default: strbuf_str(&sb, NOT_SUPPORTED_CYRILIC); return;
    }
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    strbuf_clear(&sb);
    cur_y += cgxyz.height_px;

    strbuf_str(&sb, THRESHOLDS_CYRILIC);
    strbuf_str(&sb, ": ");
    strbuf_u32(&sb, testparam->min_voltage, 1);
    strbuf_str(&sb, "...");
    strbuf_u32(&sb, testparam->max_voltage, 1);
    strbuf_str(&sb, " B");
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    strbuf_clear(&sb);
    // пропускаем 5-ю строку
    cur_y += 2 * cgxyz.height_px;

    int32_t voltage_before_div_uV = (int32_t)((float)choosed_metrics->voltage_uV.mean_value * HV_FEEDBACK_DIVIDER);
    int32_t tripple_standard_deviation_voltage_before_div_uV = (int32_t)
                (3LL * (int64_t)((float)choosed_metrics->voltage_uV.standard_deviation * HV_FEEDBACK_DIVIDER));
    if (voltage_before_div_uV < 0) {
        strbuf_char(&sb, '-');
        voltage_before_div_uV = -voltage_before_div_uV;
    }
    strbuf_u32(&sb, voltage_before_div_uV / 1000000L, 1);
    strbuf_char(&sb, '.');
    strbuf_u32(&sb, (voltage_before_div_uV % 1000000L) / 1000L, 3);
    strbuf_char(&sb, '$');
    strbuf_u32(&sb, tripple_standard_deviation_voltage_before_div_uV / 1000000L, 1);
    strbuf_char(&sb, '.');
    strbuf_u32(&sb, (tripple_standard_deviation_voltage_before_div_uV % 1000000L) / 1000L, 3);
    strbuf_str(&sb, " B");
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    strbuf_clear(&sb);
    cur_y += cgxyz.height_px;

    if (type_choosed_test != GasDischargeTube) {
        int32_t current_nA = choosed_metrics->current_nA.mean_value;
        if (current_nA < 0) {
            strbuf_char(&sb, '-');
            current_nA = -current_nA;
        }
        int32_t tripple_standard_deviation_current_nA = 3 * choosed_metrics->current_nA.standard_deviation;
        strbuf_u32(&sb, current_nA / 1000000L, 1);
        strbuf_char(&sb, '.');
        strbuf_u32(&sb, (current_nA % 1000000L) / 1000L, 3);
        strbuf_char(&sb, '$');
        strbuf_u32(&sb, tripple_standard_deviation_current_nA / 1000000L, 1);
        strbuf_char(&sb, '.');
        strbuf_u32(&sb, (tripple_standard_deviation_current_nA % 1000000L) / 1000L, 3);
        strbuf_str(&sb, " l@");
        screen_get_center_from_string(&cur_x, sb.len);
        fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    } else {
        strbuf_str(&sb, DISCHARGER_CYRILIC);
        strbuf_char(&sb, ' ');
        if (choosed_metrics->current_nA.mean_value != INT32_MAX) {
            strbuf_str(&sb, NOT_CYRILIC);
            strbuf_char(&sb, ' ');
        }
        strbuf_str(&sb, WAS_TRIGGERED_CYRILIC);
        screen_get_center_from_string(&cur_x, sb.len);
        fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    }
    return;
}

ScreenID screen_testdata_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Cancel: return ScreenTestResult;
        default: return ScreenTestData;
    }
}