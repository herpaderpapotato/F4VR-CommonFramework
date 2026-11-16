#pragma once

#include "F4VROffsets.h"
#include "f4sevr/Common.h"

namespace f4cf::f4vr
{
    class BSFlattenedBoneTree : public RE::NiNode
    {
    public:
        struct BoneTransforms
        {
            RE::NiTransform local;
            RE::NiTransform world;
            short parPos;
            short childPos;
            uint32_t unk8c;
            NiNode* refNode;
            RE::BSFixedString name;
            uint64_t unk98;
        };

        struct BoneNodePosition
        {
            F4SEVR::StringCache::Entry* name;
            int position;
            uint32_t pad;
            uintptr_t unk;
        };

        int GetBoneIndex(const std::string& a_name)
        {
            auto nameStr = new RE::BSFixedString(a_name.c_str());
            return BSFlattenedBoneTree_GetBoneIndex(this, nameStr);
        }

        NiNode* GetBoneNode(const std::string& a_name)
        {
            auto nameStr = new RE::BSFixedString(a_name.c_str());
            return BSFlattenedBoneTree_GetBoneNode(this, nameStr);
        }

        NiNode* GetBoneNode(const int a_pos)
        {
            return BSFlattenedBoneTree_GetBoneNodeFromPos(this, a_pos);
        }

        void UpdateBoneArray()
        {
            BSFlattenedBoneTree_UpdateBoneArray(this);
        }

        int numTransforms;
        uint32_t pad0;
        BoneTransforms* transforms;
        uint64_t unk190;
        uint64_t unk198;
        uint64_t unk1a0;
        uint64_t unk1a8;
        uint64_t unk1b0;
        BoneNodePosition* bonePositions;
    };

#ifdef FRIK_FLAT // C7: flat NiNode is 0x140 (VR 0x180); boneCount +0x140, FlattenedBone* +0x148, entry 0xA0 (analysis/vr-hands-weapon.md 1.2)
    static_assert(sizeof(BSFlattenedBoneTree) == 0x180);
    static_assert(offsetof(BSFlattenedBoneTree, numTransforms) == 0x140 && offsetof(BSFlattenedBoneTree, transforms) == 0x148);
    static_assert(sizeof(BSFlattenedBoneTree::BoneTransforms) == 0xA0);
#else
    static_assert(sizeof(BSFlattenedBoneTree) == 0x1C0);
#endif
}
