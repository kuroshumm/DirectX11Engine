#include "CharacterFactory.h"
#include "AnimSequenceLoader.h"

namespace Engine
{
    CharacterFactory::CharacterFactory(
        ID3D11Device* pDevice,
        entt::registry& registry,
        ModelLoader& modelLoader
    )
        : m_registry(registry),
        m_pDevice(pDevice),
        m_modelLoader(modelLoader)
    {
        m_pDefaultTexture = std::make_shared<Texture>();
        m_pDefaultTexture->CreateSolidColor(m_pDevice);
    }

    HRESULT CharacterFactory::LoadModelAsset()
    {
        ModelLoader::LoadResult loadResult = m_modelLoader.Load("YBot/Y_Bot.fbx");
        if (!loadResult.isSuccess)
        {
            return E_FAIL;
        }

        m_loadResult = std::move(loadResult);

        return S_OK;
    }

    entt::entity CharacterFactory::SpawnHumanoid(const HumanoidSpawnDesc& desc)
    {
        entt::entity entity = m_registry.create();

        TransformComponent transformComp;
        transformComp.position = desc.position;
        transformComp.rotation = desc.rotation;
        transformComp.scale = desc.scale;
        m_registry.emplace<TransformComponent>(entity, std::move(transformComp));

        ModelComponent modelComp;
        modelComp.pModelAsset = m_loadResult.modelAsset;
        m_registry.emplace<ModelComponent>(entity, std::move(modelComp));

        SkeletonComponent skeltonComp;
        skeltonComp.bones = m_loadResult.skeltonAsset->initialBones;
        skeltonComp.rootBoneIndex = m_loadResult.skeltonAsset->rootBoneIndex;
        m_registry.emplace<SkeletonComponent>(entity, std::move(skeltonComp));

        SkinningPaletteComponent skinningPaletteComp;
        ModelLoader::CreatePaletteBuffer(m_pDevice, skinningPaletteComp.pPaletteBuffer);
        m_registry.emplace<SkinningPaletteComponent>(entity, std::move(skinningPaletteComp));

        AnimationComponent animationComp;
        m_registry.emplace<AnimationComponent>(entity, std::move(animationComp));

        auto findBone = [&](std::initializer_list<const char*> names) -> int
            {
                for (const char* name : names)
                {
                    int idx = m_loadResult.skeltonAsset->GetBoneIndex(name);
                    if (idx != -1) return idx;
                }
                return -1;
            };

        IKComponent ikComp;
        int upLegIndex = findBone({ "LeftUpLeg", "thigh_l", "mixamorig:LeftUpLeg" });
        int legIndex = findBone({ "LeftLeg", "calf_l", "mixamorig:LeftLeg" });
        int footIndex = findBone({ "LeftFoot", "foot_l", "mixamorig:LeftFoot" });
        if (upLegIndex != -1 && legIndex != -1 && footIndex != -1)
        {
            ikComp.effectorBoneIndex = footIndex;
            ikComp.chainIndices = { upLegIndex, legIndex, footIndex };
            // ヒザ関節の 1D 角度制約(X 軸周りに 0°〜120°)
            ikComp.SetConstraint(legIndex, DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0.0f, 120.0f);
            m_registry.emplace<IKComponent>(entity, std::move(ikComp));
        }

        MaterialComponent material;
        material.pShader = desc.pShader;
        material.pTexture = desc.pTexture != nullptr 
            ? desc.pTexture 
            : m_pDefaultTexture;
        m_registry.emplace<MaterialComponent>(entity, std::move(material));

        VelocityComponent velocity;
        m_registry.emplace<VelocityComponent>(entity, std::move(velocity));

        // LOD
        m_registry.emplace<LODComponent>(entity);

        DamageableCollisionComponent damageableCollision;
        damageableCollision.isActive = true;
        damageableCollision.localOffset = DirectX::XMFLOAT3(0.0f, 0.85f, 0.0f);
        damageableCollision.shapeDesc = BoxShapeDesc{ DirectX::XMFLOAT3(0.22f, 0.85f, 0.22f) };
        damageableCollision.worldVolume = DirectX::BoundingOrientedBox{};
        m_registry.emplace<DamageableCollisionComponent>(entity, std::move(damageableCollision));

        m_registry.emplace<DamageRequestComponent>(entity);
        m_registry.emplace<DamageResultComponent>(entity);
        m_registry.emplace<DamageStateComponent>(entity);
        m_registry.emplace<DeadStateComponent>(entity);
        m_registry.emplace<HealthComponent>(entity);

        m_registry.emplace<CharacterLocomotionComponent>(entity);
        m_registry.emplace<CharacterMovementConfigComponent>(entity);
        m_registry.emplace<LocomotionProposalComponent>(entity);
        m_registry.emplace<CombatRequestComponent>(entity);
        m_registry.emplace<CharacterActionStateComponent>(entity);
        m_registry.emplace<CharacterCombatStateComponent>(entity);
        m_registry.emplace<CharacterActionResultComponent>(entity);
        m_registry.emplace<LocomotionAnimComponent>(entity);
        m_registry.emplace<MotionWarpComponent>(entity);
        m_registry.emplace<CombatAttackPresetComponent>(entity);
        m_registry.emplace<AnimEventComponent>(entity);

        return entity;
    }
}
