#include "AnimSequenceLoader.h"
#include "AnimationLoader.h"

namespace
{
    using namespace Engine;

    std::shared_ptr<AnimSequenceAsset> CreateIdle()
    {
        auto asset = std::make_shared<AnimSequenceAsset>();
        asset->animState = AnimStateType::IDLE;
        asset->clip = AnimationLoader::Load("YBot/BouncingFight_Idle.fbx");
        return asset;
    }

    std::shared_ptr<AnimSequenceAsset> CreateWalk()
    {
        auto asset = std::make_shared<AnimSequenceAsset>();
        asset->animState = AnimStateType::WALK;
        asset->clip = AnimationLoader::Load("YBot/StandardWalk.fbx");
        return asset;
    }

    std::shared_ptr<AnimSequenceAsset> CreateWalkRight()
    {
        auto asset = std::make_shared<AnimSequenceAsset>();
        asset->animState = AnimStateType::WALK_RIGHT;
        asset->clip = AnimationLoader::Load("YBot/Punching_Walking_Right.fbx");
        return asset;
    }

    std::shared_ptr<AnimSequenceAsset> CreateWalkLeft()
    {
        auto asset = std::make_shared<AnimSequenceAsset>();
        asset->animState = AnimStateType::WALK_LEFT;
        asset->clip = AnimationLoader::Load("YBot/Punching_Walking_Left.fbx");
        return asset;
    }

    std::shared_ptr<AnimSequenceAsset> CreateWalkBack()
    {
        auto asset = std::make_shared<AnimSequenceAsset>();
        asset->animState = AnimStateType::WALK_BACK;
        asset->clip = AnimationLoader::Load("YBot/Punching_Walking_Back.fbx");
        return asset;
    }

    std::shared_ptr<AnimSequenceAsset> CreateRun()
    {
        auto asset = std::make_shared<AnimSequenceAsset>();
        asset->animState = AnimStateType::RUN;
        asset->clip = AnimationLoader::Load("YBot/Running.fbx");
        return asset;
    }

    std::shared_ptr<AnimSequenceAsset> CreateCombo1()
    {
        auto asset = std::make_shared<AnimSequenceAsset>();
        asset->animState = AnimStateType::COMBO1;
        asset->clip = AnimationLoader::Load("YBot/Punching_Jab_left.fbx");

        // イベント区間の設定
        asset->windowEvents.push_back({
            AnimEventWindowType::ATTACK_COLLISION,
            { 0.3f, 0.7f } // 判定発生区間
            });
        asset->windowEvents.push_back({
            AnimEventWindowType::COMBO_QUEUE,
            { 0.35f, 1.0f } // 先行入力受付区間
            });
        return asset;
    }

    std::shared_ptr<AnimSequenceAsset> CreateCombo2()
    {
        auto asset = std::make_shared<AnimSequenceAsset>();
        asset->animState = AnimStateType::COMBO2;
        asset->clip = AnimationLoader::Load("YBot/Punching_Jab_right.fbx");

        asset->windowEvents.push_back({
            AnimEventWindowType::ATTACK_COLLISION,
            { 0.3f, 0.7f }
            });
        asset->windowEvents.push_back({
            AnimEventWindowType::COMBO_QUEUE,
            { 0.50f, 1.0f }
            });
        return asset;
    }

    std::shared_ptr<AnimSequenceAsset> CreateDamage()
    {
        auto asset = std::make_shared<AnimSequenceAsset>();
        asset->animState = AnimStateType::DAMAGE;
        asset->clip = AnimationLoader::Load("YBot/ReceivePunchToTheFace.fbx");

        return asset;
    }

    std::shared_ptr<AnimSequenceAsset> CreateDeath()
    {
        auto asset = std::make_shared<AnimSequenceAsset>();
        asset->animState = AnimStateType::DEATH;
        asset->clip = AnimationLoader::Load("YBot/Dying.fbx");

        return asset;
    }
}

namespace Engine
{
    std::shared_ptr<AnimSequenceAsset> AnimSequenceLoader::Load(AnimStateType stateType)
    {
        switch (stateType)
        {
        case AnimStateType::IDLE:       return CreateIdle();
        case AnimStateType::WALK:       return CreateWalk();
        case AnimStateType::WALK_RIGHT: return CreateWalkRight();
        case AnimStateType::WALK_LEFT:  return CreateWalkLeft();
        case AnimStateType::WALK_BACK:  return CreateWalkBack();
        case AnimStateType::RUN:        return CreateRun();
        case AnimStateType::COMBO1:     return CreateCombo1();
        case AnimStateType::COMBO2:     return CreateCombo2();
        case AnimStateType::DAMAGE:     return CreateDamage();
        case AnimStateType::DEATH:      return CreateDeath();
        }

        return nullptr;
    }
}
