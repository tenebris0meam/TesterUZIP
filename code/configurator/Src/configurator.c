#include "configurator.h"
#include "params.h"

#include <stdbool.h>
#include <stdint.h>

bool configurator_read_config (void) {
    if (params.magic != CONFIG_MAGIC) {
        return false;
    }

    if (params.version != CONFIG_VERSION) {
        return false;
    }

    /* Пустой каталог сделал бы бессмысленными все экраны выбора */
    if ((params.series_count == 0) || 
        (params.groups_count == 0) || 
        (params.devices_count == 0)) {
        return false;
    }

    if ((params.series_names == 0) || 
        (params.groups_names == 0) || 
        (params.devices_names == 0) || 
        (params.devices == 0)) {
        return false;
    }

    /* Последовательности могут отсутствовать, но тогда и ссылаться на них нельзя */
    if ((params.sequences_count != 0) && (params.sequences == 0)) {
        return false;
    }

    return true;
}
