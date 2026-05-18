#include "dusk/hook.hpp"
#include "dusk/mod_api.h"

#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_warp_bug.h"


extern "C" {

static DuskModAPI* gApi;
static daWarpBug_c* puppet;
static daAlink_c* link;

static void on_create_post(void* ret, void* args) {
    link = daAlink_getAlinkActorClass();
    puppet = reinterpret_cast<daWarpBug_c*>(ret);
    gApi->log_info("puppet init: %p", puppet);
}

static void on_execute_post(void* ret, void* args) {
    puppet->current.pos = link->current.pos;
    gApi->log_info("puppet ontick: %f", puppet->current.pos.x);
}

static void on_draw_post(void* ret, void* args) {
    puppet->current.pos = link->current.pos;
    gApi->log_info("puppet draw: %f", puppet->current.pos.y);
}

void mod_init(DuskModAPI* api) {
    gApi = api;

    dusk::init(api);
    api->log_info("Dusklight-Online initialized.");

    // Setup puppet hooks
    dusk::hookAddPost<&daWarpBug_c::create>(on_create_post);
    dusk::hookAddPost<&daWarpBug_c::execute>(on_execute_post);
    dusk::hookAddPost<&daWarpBug_c::draw>(on_draw_post);
}

void mod_tick(DuskModAPI* api) {
    (void)api;
}

void mod_cleanup(DuskModAPI* api) {
    (void)api;
}
}
