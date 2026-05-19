#include "mod/online_puppet_actor.hpp"

#include "JSystem/J3DGraphAnimator/J3DModel.h"
#include "JSystem/J3DGraphAnimator/J3DJoint.h"
#include "SSystem/SComponent/c_xyz.h"
#include "d/actor/d_a_alink.h"
#include "f_op/f_op_actor_mng.h"
#include "m_Do/m_Do_ext.h"

#include <cmath>
#include <cstdlib>

struct OnlinePuppetActor_State {
    u32 frameCount;
    J3DModel* bodyModel;
    J3DModel* faceModel;
    J3DModel* headModel;
    J3DModel* handModel;
};

static DuskActorTypeId sPuppetType;

struct OnlinePuppetActor_JointState {
    J3DJointCallBack callback;
    J3DMtxCalc* mtxCalc;
};

static J3DModel* OnlinePuppetActor_CloneModel(J3DModel* source_model) {
    if (source_model == nullptr) {
        return nullptr;
    }

    J3DModelData* model_data = source_model->getModelData();
    if (model_data == nullptr) {
        return nullptr;
    }

    return mDoExt_J3DModel__create(model_data, 0x80000, 0x11000084);
}

static u16 OnlinePuppetActor_GetJointCount(J3DModel* model) {
    if (model == nullptr || model->getModelData() == nullptr) {
        return 0;
    }

    return model->getModelData()->getJointNum();
}

static MtxP OnlinePuppetActor_GetAnmMtx(J3DModel* model, u16 joint_no) {
    if (joint_no >= OnlinePuppetActor_GetJointCount(model)) {
        return nullptr;
    }

    return model->getAnmMtx(joint_no);
}

static void OnlinePuppetActor_CalcRestPose(J3DModel* model) {
    if (model == nullptr || model->getModelData() == nullptr) {
        return;
    }

    u16 joint_count = model->getModelData()->getJointNum();
    if (joint_count == 0) {
        model->calc();
        return;
    }

    OnlinePuppetActor_JointState* saved_state = static_cast<OnlinePuppetActor_JointState*>(
        malloc(sizeof(OnlinePuppetActor_JointState) * joint_count));
    if (saved_state == nullptr) {
        return;
    }

    for (u16 joint_no = 0; joint_no < joint_count; joint_no++) {
        J3DJoint* joint = model->getModelData()->getJointNodePointer(joint_no);
        saved_state[joint_no].callback = joint->getCallBack();
        saved_state[joint_no].mtxCalc = joint->getMtxCalc();
        joint->setCallBack(nullptr);
        joint->setMtxCalc(nullptr);
    }

    model->calc();

    for (u16 joint_no = 0; joint_no < joint_count; joint_no++) {
        J3DJoint* joint = model->getModelData()->getJointNodePointer(joint_no);
        joint->setCallBack(saved_state[joint_no].callback);
        joint->setMtxCalc(saved_state[joint_no].mtxCalc);
    }

    free(saved_state);
}

static void OnlinePuppetActor_DeleteModel(J3DModel** model) {
    if (model == nullptr || *model == nullptr) {
        return;
    }

    // Model clone allocations are owned by the mod actor heap requested at registration.
    *model = nullptr;
}

static f32 OnlinePuppetActor_GetScale(f32 scale) {
    if (scale == 0.0f) {
        return 1.0f;
    }

    return scale;
}

static void OnlinePuppetActor_CopyMtx(MtxP destination, MtxP source) {
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 4; column++) {
            destination[row][column] = source[row][column];
        }
    }
}

static void OnlinePuppetActor_BuildBaseMtx(fopAc_ac_c* actor, Mtx out_mtx) {
    constexpr f32 kAngleToRadians = 6.28318530717958647692f / 65536.0f;

    f32 scale_x = OnlinePuppetActor_GetScale(actor->scale.x);
    f32 scale_y = OnlinePuppetActor_GetScale(actor->scale.y);
    f32 scale_z = OnlinePuppetActor_GetScale(actor->scale.z);
    f32 angle_y = static_cast<f32>(actor->shape_angle.y) * kAngleToRadians;
    f32 sin_y = std::sin(angle_y);
    f32 cos_y = std::cos(angle_y);

    out_mtx[0][0] = cos_y * scale_x;
    out_mtx[0][1] = 0.0f;
    out_mtx[0][2] = sin_y * scale_z;
    out_mtx[0][3] = actor->current.pos.x;

    out_mtx[1][0] = 0.0f;
    out_mtx[1][1] = scale_y;
    out_mtx[1][2] = 0.0f;
    out_mtx[1][3] = actor->current.pos.y;

    out_mtx[2][0] = -sin_y * scale_x;
    out_mtx[2][1] = 0.0f;
    out_mtx[2][2] = cos_y * scale_z;
    out_mtx[2][3] = actor->current.pos.z;
}

static void OnlinePuppetActor_UpdateMatrices(fopAc_ac_c* actor, OnlinePuppetActor_State* state) {
    if (actor == nullptr || state == nullptr || state->bodyModel == nullptr) {
        return;
    }

    Mtx base_mtx;
    OnlinePuppetActor_BuildBaseMtx(actor, base_mtx);
    OnlinePuppetActor_CopyMtx(state->bodyModel->getBaseTRMtx(), base_mtx);
    OnlinePuppetActor_CalcRestPose(state->bodyModel);

    if (state->faceModel != nullptr) {
        MtxP head_mtx = OnlinePuppetActor_GetAnmMtx(state->bodyModel, 4);
        if (head_mtx != nullptr) {
            OnlinePuppetActor_CopyMtx(state->faceModel->getBaseTRMtx(), head_mtx);
        }
        OnlinePuppetActor_CalcRestPose(state->faceModel);
    }

    if (state->headModel != nullptr) {
        MtxP head_mtx = OnlinePuppetActor_GetAnmMtx(state->bodyModel, 4);
        if (head_mtx != nullptr) {
            OnlinePuppetActor_CopyMtx(state->headModel->getBaseTRMtx(), head_mtx);
        }
        OnlinePuppetActor_CalcRestPose(state->headModel);
    }

    if (state->handModel != nullptr) {
        OnlinePuppetActor_CopyMtx(state->handModel->getBaseTRMtx(), state->bodyModel->getBaseTRMtx());
        OnlinePuppetActor_CalcRestPose(state->handModel);

        MtxP left_hand_mtx = OnlinePuppetActor_GetAnmMtx(state->bodyModel, 9);
        MtxP right_hand_mtx = OnlinePuppetActor_GetAnmMtx(state->bodyModel, 0xE);
        MtxP puppet_left_hand_mtx = OnlinePuppetActor_GetAnmMtx(state->handModel, 1);
        MtxP puppet_right_hand_mtx = OnlinePuppetActor_GetAnmMtx(state->handModel, 2);
        if (left_hand_mtx != nullptr && puppet_left_hand_mtx != nullptr) {
            OnlinePuppetActor_CopyMtx(puppet_left_hand_mtx, left_hand_mtx);
        }
        if (right_hand_mtx != nullptr && puppet_right_hand_mtx != nullptr) {
            OnlinePuppetActor_CopyMtx(puppet_right_hand_mtx, right_hand_mtx);
        }
    }

    actor->model = state->bodyModel;
    fopAcM_SetMtx(actor, state->bodyModel->getBaseTRMtx());
}

static void OnlinePuppetActor_DrawModel(fopAc_ac_c* actor, J3DModel* model) {
    if (actor == nullptr || model == nullptr) {
        return;
    }

    mDoExt_modelEntryDL(model);
}

static int32_t OnlinePuppetActor_Create(DuskActorHandle actor_handle, void* state, void* userdata) {
    DuskModAPI* api = static_cast<DuskModAPI*>(userdata);
    fopAc_ac_c* actor = static_cast<fopAc_ac_c*>(api->actor_get_game_actor(actor_handle));
    if (actor == nullptr) {
        return 0;
    }

    OnlinePuppetActor_State* puppet_state = static_cast<OnlinePuppetActor_State*>(state);
    puppet_state->frameCount = 0;
    puppet_state->bodyModel = nullptr;
    puppet_state->faceModel = nullptr;
    puppet_state->headModel = nullptr;
    puppet_state->handModel = nullptr;

    daAlink_c* link = daAlink_getAlinkActorClass();
    if (link == nullptr || link->mpLinkModel == nullptr) {
        api->log_error("OnlinePuppet could not find Link model data");
        return 0;
    }

    puppet_state->bodyModel = OnlinePuppetActor_CloneModel(link->mpLinkModel);
    puppet_state->faceModel = OnlinePuppetActor_CloneModel(link->mpLinkFaceModel);
    puppet_state->headModel = OnlinePuppetActor_CloneModel(link->mpLinkHatModel);
    puppet_state->handModel = OnlinePuppetActor_CloneModel(link->mpLinkHandModel);
    if (puppet_state->bodyModel == nullptr) {
        api->log_error("OnlinePuppet could not create body model");
        return 0;
    }

    actor->health = 1;
    actor->shape_angle = actor->current.angle;
    actor->attention_info.position = actor->current.pos;
    actor->eyePos = actor->current.pos;
    actor->eyePos.y += 150.0f;
    actor->attention_info.position.y += 150.0f;
    fopAcM_SetMin(actor, -60.0f, 0.0f, -60.0f);
    fopAcM_SetMax(actor, 60.0f, 180.0f, 60.0f);
    fopAcM_setCullSizeFar(actor, 1.0f);
    OnlinePuppetActor_UpdateMatrices(actor, puppet_state);
    return 1;
}

static int32_t OnlinePuppetActor_Destroy(DuskActorHandle actor_handle, void* state, void* userdata) {
    (void)actor_handle;
    (void)userdata;

    OnlinePuppetActor_State* puppet_state = static_cast<OnlinePuppetActor_State*>(state);
    if (puppet_state == nullptr) {
        return 1;
    }

    OnlinePuppetActor_DeleteModel(&puppet_state->handModel);
    OnlinePuppetActor_DeleteModel(&puppet_state->headModel);
    OnlinePuppetActor_DeleteModel(&puppet_state->faceModel);
    OnlinePuppetActor_DeleteModel(&puppet_state->bodyModel);
    return 1;
}

static int32_t OnlinePuppetActor_Execute(DuskActorHandle actor_handle, void* state, void* userdata) {
    DuskModAPI* api = static_cast<DuskModAPI*>(userdata);
    fopAc_ac_c* actor = static_cast<fopAc_ac_c*>(api->actor_get_game_actor(actor_handle));
    if (actor == nullptr) {
        return 0;
    }

    OnlinePuppetActor_State* puppet_state = static_cast<OnlinePuppetActor_State*>(state);
    puppet_state->frameCount++;

    actor->attention_info.position = actor->current.pos;
    actor->attention_info.position.y += 150.0f;
    actor->eyePos = actor->current.pos;
    actor->eyePos.y += 150.0f;
    OnlinePuppetActor_UpdateMatrices(actor, puppet_state);
    return 1;
}

static int32_t OnlinePuppetActor_Draw(DuskActorHandle actor_handle, void* state, void* userdata) {
    DuskModAPI* api = static_cast<DuskModAPI*>(userdata);
    fopAc_ac_c* actor = static_cast<fopAc_ac_c*>(api->actor_get_game_actor(actor_handle));
    OnlinePuppetActor_State* puppet_state = static_cast<OnlinePuppetActor_State*>(state);
    if (actor == nullptr || puppet_state == nullptr || puppet_state->bodyModel == nullptr) {
        return 0;
    }

    OnlinePuppetActor_DrawModel(actor, puppet_state->bodyModel);
    OnlinePuppetActor_DrawModel(actor, puppet_state->handModel);
    OnlinePuppetActor_DrawModel(actor, puppet_state->headModel);
    OnlinePuppetActor_DrawModel(actor, puppet_state->faceModel);
    return 1;
}

void OnlinePuppetActor_Register(DuskModAPI* api) {
    DuskModActorDesc desc = {};
    desc.struct_size = sizeof(desc);
    desc.name = "OnlinePuppet";
    desc.state_size = sizeof(OnlinePuppetActor_State);
    desc.state_align = alignof(OnlinePuppetActor_State);
    desc.create = OnlinePuppetActor_Create;
    desc.destroy = OnlinePuppetActor_Destroy;
    desc.execute = OnlinePuppetActor_Execute;
    desc.draw = OnlinePuppetActor_Draw;
    desc.userdata = api;
    desc.heap_size = 0x80000;

    sPuppetType = api->actor_register_type(&desc);
    if (sPuppetType == 0) {
        api->log_error("failed to register OnlinePuppet actor");
    }
}

DuskProcId OnlinePuppetActor_SpawnLocal(DuskModAPI* api) {
    if (sPuppetType == 0) {
        return 0xFFFFFFFFu;
    }

    DuskModActorSpawnInfo spawn = {};
    spawn.struct_size = sizeof(spawn);
    spawn.type_id = sPuppetType;
    spawn.scale_x = 1.0f;
    spawn.scale_y = 1.0f;
    spawn.scale_z = 1.0f;
    spawn.argument = -1;
    spawn.room_no = -1;

    daAlink_c* link = daAlink_getAlinkActorClass();
    if (link != nullptr) {
        spawn.pos_x = link->current.pos.x;
        spawn.pos_y = link->current.pos.y;
        spawn.pos_z = link->current.pos.z;
        spawn.room_no = link->current.roomNo;
        spawn.angle_x = link->shape_angle.x;
        spawn.angle_y = link->shape_angle.y;
        spawn.angle_z = link->shape_angle.z;
    }

    return api->actor_spawn(&spawn);
}
