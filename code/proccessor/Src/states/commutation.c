#include "states/commutation.h"
#include "commutator.h"
#include "display.h"
#include "proccessor.h"
#include "model.h"

void proccessor_commutation_on_enter_checkdevice (void) {
    // Выключаем ввод
    // inputevent_deny_input();
    
    if (test_runtime.current_sequence == NULL) {
        proccessor_raise_event(TestSequenceFinished);
        return;
    }
    if (!proccessor_more_tests_left()) {
        proccessor_raise_event(TestSequenceFinished);
        return;
    }

    /*
     * Единственное место, где экран перерисовывается во время прогона.
     * Во время генерации этого делать не стоит: шину SPI2 в этот момент
     * каждые 100 мкс занимает INA239, а показывать нечего - на экране
     * прогона меняются только номера выводов и счётчик проверок, то есть
     * ровно то, что обновляется здесь, между проверками.
     */
    display_allow_render();

    if (!proccessor_setup_lines()) {
        /*
         * Проверку выполнить нечем. Засчитываем провал и НЕ подаём напряжение;
         * следующий проход возьмёт следующую позицию, а когда они кончатся -
         * поднимет TestSequenceFinished ветвью выше.
         */
        // proccessor_record_result(false);
        model_handle_test_result();
        return;
    }

    commutator_allow_commutation();
    return;
}
#ifdef TESTER_MODES
void proccessor_commutation_on_enter_probe (void) {
    commutator_set_active_interface(Probe);
    commutator_allow_commutation();
    return;
}
#endif

void proccessor_commutation_on_exit (void) {
    display_deny_render();
    return;
}

void proccessor_commutation_proccess_default (void) {
    proccessor_raise_event(CommutationComplete);
    return;
}

State proccessor_commutation_handle_event_default (Event event) {
    switch (event) {
        case CommutationComplete: return Generation;

        case TestSequenceFinished: return TestSequenceComplete;

        case ReturnToIdle:
            commutator_deny_commutation();
            return Idle_Setting;

        case AlertActive: return HardwareFail;

        default: return Commutation;
    }
}