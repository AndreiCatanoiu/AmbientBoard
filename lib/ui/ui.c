#include "ui.h"
#include "ui_theme.h"
#include "ui_router.h"

void ui_init(void)
{
    ui_theme_apply_saved();
    ui_router_init();
}
