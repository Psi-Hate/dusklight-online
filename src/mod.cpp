#include "dusk/hook.hpp"
#include "dusk/mod_api.h"

#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_warp_bug.h"

static DuskModAPI* gApi;
static daWarpBug_c* puppet;
static daAlink_c* link;

static request_of_phase_process_class mPhase;
static J3DModel* mpModel;
static Mtx* mCullMtx;
static u32 mpWarpTexData;
static dKy_tevstr_c mTevStr;

Mtx mDoMtx_stack_c::now;

#include "SSystem/SComponent/c_lib.h"
#include "SSystem/SComponent/c_math.h"
#include "dolphin/types.h"

void mDoMtx_stack_c::ZXYrotM(csXyz const& param_0) {
    ZXYrotM(param_0.x, param_0.y, param_0.z);
}

const J3DLightInfo j3dDefaultLightInfo = {
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    -1.0f,
    0.0f,
    0xff,
    0xff,
    0xff,
    0xff,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
};

void C_MTXTrans(Mtx m, f32 xT, f32 yT, f32 zT) {
    m[0][0] = 1;
    m[0][1] = 0;
    m[0][2] = 0;
    m[0][3] = xT;
    m[1][0] = 0;
    m[1][1] = 1;
    m[1][2] = 0;
    m[1][3] = yT;
    m[2][0] = 0;
    m[2][1] = 0;
    m[2][2] = 1;
    m[2][3] = zT;
}

void C_MTXCopy(const Mtx src, Mtx dst) {
    if (src != dst) {
        dst[0][0] = src[0][0];
        dst[0][1] = src[0][1];
        dst[0][2] = src[0][2];
        dst[0][3] = src[0][3];
        dst[1][0] = src[1][0];
        dst[1][1] = src[1][1];
        dst[1][2] = src[1][2];
        dst[1][3] = src[1][3];
        dst[2][0] = src[2][0];
        dst[2][1] = src[2][1];
        dst[2][2] = src[2][2];
        dst[2][3] = src[2][3];
    }
}

J3DModel* puppet_initModel(J3DModelData* i_modelData, u32 mdlFlags, u32 diffFlags) {
    J3DTexture* tex = i_modelData->getTexture();
    int texNo = tex->getNum() - 1;

    int warpMaterial = false;
    if (texNo >= 0) {
        ResTIMG* timg = tex->getResTIMG(texNo);

        if (mpWarpTexData == (u32)timg + timg->imageOffset) {
            warpMaterial = true;
        }
    }

    if (warpMaterial) {
        dRes_info_c::onWarpMaterial(i_modelData);
        diffFlags |= 0x2000400;
    }

    J3DModel* model = mDoExt_J3DModel__create(i_modelData, mdlFlags, diffFlags | 0x11000084);

    if (warpMaterial) {
        dRes_info_c::offWarpMaterial(i_modelData);
    }

    return model;
}

void puppet_delete() {
    dComIfG_resDelete(&puppet->mPhase, "Bmdl");
}

static int useHeapInit(fopAc_ac_c* i_actor) {
    ResTIMG* timg = (ResTIMG*)dComIfG_getObjectRes("Always", 0x5D);
    mpWarpTexData = (u32)timg + timg->imageOffset;
    mpModel = puppet_initModel((J3DModelData*)dComIfG_getObjectRes("Bmdl", 0xF), 0x80000, 0);

    if (mpModel == NULL) {
        return 0;
    }

    return 1;
}

static int32_t on_create_pre(void* args) {
    puppet = dusk::arg<daWarpBug_c*>(args, 0);
    link = daAlink_getAlinkActorClass();

    int phase_state = dComIfG_resLoad(&puppet->mPhase, "Bmdl");
    if (phase_state == cPhs_COMPLEATE_e) {
        mCullMtx = &mpModel->getBaseTRMtx();
    }

    gApi->log_info("puppet create: %p", puppet);
    return 0;
}

static int32_t on_execute_pre(void* args) {
    mDoMtx_stack_c::transS(puppet->current.pos.x, puppet->current.pos.y, puppet->current.pos.z);
    mDoMtx_stack_c::ZXYrotM(puppet->shape_angle.x, puppet->shape_angle.y, puppet->shape_angle.z);
    PSMTXCopy(mDoMtx_stack_c::now, mpModel->mBaseTransformMtx);
    mpModel->calc();

    gApi->log_info("puppet ontick: %f", puppet->current.pos.x);
    return 1;
}

static int32_t on_draw_pre(void* args) {
    J3DModel* model = mpModel;

    g_env_light.settingTevStruct(0, &puppet->current.pos, &mTevStr);
    g_env_light.setLightTevColorType_MAJI(model->getModelData(), &mTevStr);
    mDoExt_modelEntryDL(model);

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