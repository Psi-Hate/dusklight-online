#include "mod/online_puppet_actor.hpp"

#include "dusk/mod_api.h"

DUSK_REQUIRE_API_VERSION

static DuskModAPI* sApi;
static DuskProcId sLastPuppetId = 0xFFFFFFFFu;

static void Spawn_Puppet_Button(void* userdata) {
    DuskModAPI* api = static_cast<DuskModAPI*>(userdata);
    sLastPuppetId = OnlinePuppetActor_SpawnLocal(api);

    if (sLastPuppetId == 0xFFFFFFFFu) {
        api->log_error("OnlinePuppet spawn failed");
        return;
    }

    api->log_info("OnlinePuppet spawn requested: process %u", sLastPuppetId);
}

static void Build_Mod_Tab(DuskPanelHandle panel, void* userdata) {
    DuskModAPI* api = static_cast<DuskModAPI*>(userdata);

    api->panel_add_section(panel, "Online");
    api->panel_add_button(panel, "Spawn Puppet", Spawn_Puppet_Button, api);
    api->panel_add_badge_row(panel, "Puppet actor ABI", sLastPuppetId != 0xFFFFFFFFu);
}

extern "C" void mod_init(DuskModAPI* api) {
    sApi = api;

    OnlinePuppetActor_Register(api);
    api->register_tab_content(Build_Mod_Tab, api);
    api->log_info("Dusklight Online initialized");
}

extern "C" void mod_tick(DuskModAPI* api) {
    (void)api;
}

extern "C" void mod_cleanup(DuskModAPI* api) {
    (void)api;
    sApi = nullptr;
    sLastPuppetId = 0xFFFFFFFFu;
}
