#include "communication.h"
#include "statemachine.h"
#include "mdf_common.h"

static const char *TAG = "main";

void app_main(){
    MDF_LOGW("I app_main, innan comSetUp()...");
    comSetUp();
    set_up_state_machine();
}