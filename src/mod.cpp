#include "dusk/hook.hpp"
#include "dusk/mod_api.h"

#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_warp_bug.h"

extern "C" {

static DuskModAPI* gApi;
static daWarpBug_c* puppet;
static daAlink_c* link;

static int32_t on_create_pre(void* args) {
    puppet = dusk::arg<daWarpBug_c*>(args, 0);
    link = daAlink_getAlinkActorClass();
    gApi->log_info("puppet create: %p", puppet);
    return 0;
}

static int32_t on_execute_pre(void* args) {
    puppet->current.pos = link->current.pos;
    gApi->log_info("puppet ontick: %f", puppet->current.pos.x);
    return 1;
}

static int32_t on_draw_pre(void* args) {
    puppet->current.pos = link->current.pos;
    gApi->log_info("puppet draw: %f", puppet->current.pos.y);
    return 1;
}

void mod_init(DuskModAPI* api) {
    gApi = api;

    dusk::init(api);
    api->log_info("Dusklight-Online initialized.");

    // Setup puppet hooks
    dusk::hookAddPre<&daWarpBug_c::create>(on_create_pre);
    dusk::hookAddPre<&daWarpBug_c::execute>(on_execute_pre);
    dusk::hookAddPre<&daWarpBug_c::draw>(on_draw_pre);
}

void mod_tick(DuskModAPI* api) {
    (void)api;
}

void mod_cleanup(DuskModAPI* api) {
    (void)api;
}
}
