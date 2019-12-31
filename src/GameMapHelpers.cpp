#include "stdafx.h"
#include "GameMapHelpers.h"
#include "SpriteManager.h"
#include "GameMapManager.h"

bool GameMapHelpers::BuildMapMesh(GameMapManager& cityScape, const Rect2D& area, int layerIndex, MapMeshData& meshData)
{
    debug_assert(layerIndex > -1 && layerIndex < MAP_LAYERS_COUNT);

    // preallocate
    meshData.mBlocksIndices.reserve(1 * 1024 * 1024);
    meshData.mBlocksVertices.reserve(1 * 1024 * 1024);

    // prepare
    for (int tiley = 0; tiley < area.h; ++tiley)
    for (int tilex = 0; tilex < area.w; ++tilex)
    {
        if (BlockStyle* blockInfo = cityScape.GetBlockClamp(tilex + area.x, tiley + area.y, layerIndex))
        {
            for (int iface = 0; iface < eBlockFace_COUNT; ++iface)
            {
                if (blockInfo->mFaces[iface] == 0)
                    continue;

                eBlockFace faceid = (eBlockFace) iface;
                PutBlockFace(cityScape, meshData, tilex + area.x, tiley + area.y, layerIndex, faceid, blockInfo);
            }
        }
    }
    return true;
}

bool GameMapHelpers::BuildMapMesh(GameMapManager& cityScape, const Rect2D& area, MapMeshData& meshData)
{
    // preallocate
    meshData.mBlocksIndices.reserve(4 * 1024 * 1024);
    meshData.mBlocksVertices.reserve(4 * 1024 * 1024);

    // prepare
    for (int tilez = 0; tilez < MAP_LAYERS_COUNT; ++tilez)
    for (int tiley = 0; tiley < area.h; ++tiley)
    for (int tilex = 0; tilex < area.w; ++tilex)
    {
        if (BlockStyle* blockInfo = cityScape.GetBlockClamp(tilex + area.x, tiley + area.y, tilez))
        {
            for (int iface = 0; iface < eBlockFace_COUNT; ++iface)
            {
                if (blockInfo->mFaces[iface] == 0)
                    continue;

                eBlockFace faceid = (eBlockFace) iface;
                PutBlockFace(cityScape, meshData, tilex + area.x, tiley + area.y, tilez, faceid, blockInfo);
            }
        }
    }
    return true;
}

void GameMapHelpers::PutBlockFace(GameMapManager& cityScape, MapMeshData& meshData, int x, int y, int z, eBlockFace face, BlockStyle* blockInfo)
{
    assert(blockInfo && blockInfo->mFaces[face]);
    eBlockType blockType = (face == eBlockFace_Lid) ? eBlockType_Lid : eBlockType_Side;

    const int blockTexIndex = cityScape.mStyleData.GetBlockTextureLinearIndex(blockType, blockInfo->mFaces[face]);

    // setup base cube vertices
    glm::vec3 cubePoints[] =
    {
        // front face, cw
        { 0.0f,             MAP_BLOCK_LENGTH,   MAP_BLOCK_LENGTH }, 
        { MAP_BLOCK_LENGTH, MAP_BLOCK_LENGTH,   MAP_BLOCK_LENGTH }, 
        { MAP_BLOCK_LENGTH, 0.0f,               MAP_BLOCK_LENGTH }, 
        { 0.0f,             0.0f,               MAP_BLOCK_LENGTH },
        // back face, cw
        { 0.0f,             MAP_BLOCK_LENGTH,   0.0f }, 
        { MAP_BLOCK_LENGTH, MAP_BLOCK_LENGTH,   0.0f }, 
        { MAP_BLOCK_LENGTH, 0.0f,               0.0f }, 
        { 0.0f,             0.0f,               0.0f },
    };

    glm::vec3 texCoords[4] =
    {
        {0.0f, 0.0f, blockTexIndex * 1.0f},
        {1.0f, 0.0f, blockTexIndex * 1.0f},
        {1.0f, 1.0f, blockTexIndex * 1.0f},
        {0.0f, 1.0f, blockTexIndex * 1.0f}
    };

    // process slope
    const int slope = blockInfo->mSlopeType;
    switch (slope)
    {
        // N, 26 low, high
        case 1: case 2:
            cubePoints[0].y = cubePoints[1].y = ((slope - 1) / 2.0f) * MAP_BLOCK_LENGTH;
            cubePoints[4].y = cubePoints[5].y = ((slope - 1 + 1) / 2.0f) * MAP_BLOCK_LENGTH;
        break;
        // S, 26 low, high
        case 3: case 4:
            cubePoints[4].y = cubePoints[5].y = ((slope - 3) / 2.0f) * MAP_BLOCK_LENGTH;
            cubePoints[0].y = cubePoints[1].y = ((slope - 3 + 1) / 2.0f) * MAP_BLOCK_LENGTH;
        break;
        // W, 26 low, high
        case 5: case 6:
            cubePoints[1].y = cubePoints[5].y = ((slope - 5) / 2.0f) * MAP_BLOCK_LENGTH;
            cubePoints[0].y = cubePoints[4].y = ((slope - 5 + 1) / 2.0f) * MAP_BLOCK_LENGTH;
        break;
        // E, 26 low, high
        case 7: case 8:
            cubePoints[0].y = cubePoints[4].y = ((slope - 7) / 2.0f) * MAP_BLOCK_LENGTH;
            cubePoints[1].y = cubePoints[5].y = ((slope - 7 + 1) / 2.0f) * MAP_BLOCK_LENGTH;
        break;
        // N, 7 low - high
        case 9: case 10: case 11: case 12:
        case 13: case 14: case 15: case 16:
            cubePoints[0].y = cubePoints[1].y = ((slope - 9) / 8.0f) * MAP_BLOCK_LENGTH;
            cubePoints[4].y = cubePoints[5].y = ((slope - 9 + 1) / 8.0f) * MAP_BLOCK_LENGTH;
        break;
        // S, 7 low - high
        case 17: case 18: case 19: case 20:
        case 21: case 22: case 23: case 24:
            cubePoints[4].y = cubePoints[5].y = ((slope - 17) / 8.0f) * MAP_BLOCK_LENGTH;
            cubePoints[0].y = cubePoints[1].y = ((slope - 17 + 1) / 8.0f) * MAP_BLOCK_LENGTH;
        break;
        // W, 7 low - high
        case 25: case 26: case 27: case 28:
        case 29: case 30: case 31: case 32:
            cubePoints[1].y = cubePoints[5].y = ((slope - 25) / 8.0f) * MAP_BLOCK_LENGTH;
            cubePoints[0].y = cubePoints[4].y = ((slope - 25 + 1) / 8.0f) * MAP_BLOCK_LENGTH;
        break;
        // E, 7 low - high
        case 33: case 34: case 35: case 36:
        case 37: case 38: case 39: case 40:
            cubePoints[0].y = cubePoints[4].y = ((slope - 33) / 8.0f) * MAP_BLOCK_LENGTH;
            cubePoints[1].y = cubePoints[5].y = ((slope - 33 + 1) / 8.0f) * MAP_BLOCK_LENGTH;
        break;
        // 41 - 44 = 45 N,S,W,E
        case 41: cubePoints[0].y = cubePoints[1].y = 0.0f; break;
        case 42: cubePoints[4].y = cubePoints[5].y = 0.0f; break;
        case 43: cubePoints[1].y = cubePoints[5].y = 0.0f; break;
        case 44: cubePoints[0].y = cubePoints[4].y = 0.0f; break;
    }

    const int rotateLid = (face == eBlockFace_Lid) ? blockInfo->mLidRotation : 0;
    const int baseVertexIndex = meshData.mBlocksVertices.size();
    meshData.mBlocksVertices.resize(baseVertexIndex + 4);
    meshData.mBlocksVertices[baseVertexIndex + ((rotateLid + 0) % 4)].mTexcoord = texCoords[0];
    meshData.mBlocksVertices[baseVertexIndex + ((rotateLid + 1) % 4)].mTexcoord = texCoords[1];
    meshData.mBlocksVertices[baseVertexIndex + ((rotateLid + 2) % 4)].mTexcoord = texCoords[2];
    meshData.mBlocksVertices[baseVertexIndex + ((rotateLid + 3) % 4)].mTexcoord = texCoords[3];

    if (face != eBlockFace_Lid)
    {
        bool flipLeftRightFaces = ((blockInfo->mIsFlat != blockInfo->mFlipLeftRightFaces) && (face == eBlockFace_E)) ||
            (blockInfo->mFlipLeftRightFaces && (face == eBlockFace_W));

        if (flipLeftRightFaces)
        {
            std::swap(meshData.mBlocksVertices[baseVertexIndex + 0].mTexcoord, meshData.mBlocksVertices[baseVertexIndex + 1].mTexcoord);
            std::swap(meshData.mBlocksVertices[baseVertexIndex + 2].mTexcoord, meshData.mBlocksVertices[baseVertexIndex + 3].mTexcoord);
        }

        bool flipTopBottomFaces = ((blockInfo->mIsFlat != blockInfo->mFlipTopBottomFaces) && (face == eBlockFace_S)) ||
            (blockInfo->mFlipTopBottomFaces && (face == eBlockFace_N));

        if (flipTopBottomFaces)
        {
            std::swap(meshData.mBlocksVertices[baseVertexIndex + 0].mTexcoord, meshData.mBlocksVertices[baseVertexIndex + 1].mTexcoord);
            std::swap(meshData.mBlocksVertices[baseVertexIndex + 2].mTexcoord, meshData.mBlocksVertices[baseVertexIndex + 3].mTexcoord);
        }
    }

    // color
    int remap = (face == eBlockFace_Lid) ? blockInfo->mRemap : 0;
    meshData.mBlocksVertices[baseVertexIndex + 0].SetColorData(remap, blockInfo->mIsFlat ? 1 : 0);
    meshData.mBlocksVertices[baseVertexIndex + 1].SetColorData(remap, blockInfo->mIsFlat ? 1 : 0);
    meshData.mBlocksVertices[baseVertexIndex + 2].SetColorData(remap, blockInfo->mIsFlat ? 1 : 0);
    meshData.mBlocksVertices[baseVertexIndex + 3].SetColorData(remap, blockInfo->mIsFlat ? 1 : 0);

    // setup face vertices
    const glm::vec3 cubeOffset { x * MAP_BLOCK_LENGTH, z * MAP_BLOCK_LENGTH, y * MAP_BLOCK_LENGTH };
    if (face == eBlockFace_Lid)
    {
        meshData.mBlocksVertices[baseVertexIndex + 0].mPosition = cubePoints[4] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 1].mPosition = cubePoints[5] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 2].mPosition = cubePoints[1] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 3].mPosition = cubePoints[0] + cubeOffset;
    }
    if (face == eBlockFace_S)
    {
        meshData.mBlocksVertices[baseVertexIndex + 0].mPosition = cubePoints[0] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 1].mPosition = cubePoints[1] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 2].mPosition = cubePoints[2] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 3].mPosition = cubePoints[3] + cubeOffset;
    }
    if (face == eBlockFace_N)
    {
        meshData.mBlocksVertices[baseVertexIndex + 0].mPosition = cubePoints[5] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 1].mPosition = cubePoints[4] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 2].mPosition = cubePoints[7] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 3].mPosition = cubePoints[6] + cubeOffset;
    }
    if (face == eBlockFace_W)
    {
        meshData.mBlocksVertices[baseVertexIndex + 0].mPosition = cubePoints[4] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 1].mPosition = cubePoints[0] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 2].mPosition = cubePoints[3] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 3].mPosition = cubePoints[7] + cubeOffset;
    }
    if (face == eBlockFace_E)
    {
        meshData.mBlocksVertices[baseVertexIndex + 0].mPosition = cubePoints[1] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 1].mPosition = cubePoints[5] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 2].mPosition = cubePoints[6] + cubeOffset;
        meshData.mBlocksVertices[baseVertexIndex + 3].mPosition = cubePoints[2] + cubeOffset;
    }

    if (blockInfo->mIsFlat)
    {
        // should draw at W position
        if (face == eBlockFace_E)
        {
            meshData.mBlocksVertices[baseVertexIndex + 0].mPosition = cubePoints[0] + cubeOffset;
            meshData.mBlocksVertices[baseVertexIndex + 1].mPosition = cubePoints[4] + cubeOffset;
            meshData.mBlocksVertices[baseVertexIndex + 2].mPosition = cubePoints[7] + cubeOffset;
            meshData.mBlocksVertices[baseVertexIndex + 3].mPosition = cubePoints[3] + cubeOffset;
        }
        // should draw at N position
        if (face == eBlockFace_S)
        {
            meshData.mBlocksVertices[baseVertexIndex + 0].mPosition = cubePoints[4] + cubeOffset;
            meshData.mBlocksVertices[baseVertexIndex + 1].mPosition = cubePoints[5] + cubeOffset;
            meshData.mBlocksVertices[baseVertexIndex + 2].mPosition = cubePoints[6] + cubeOffset;
            meshData.mBlocksVertices[baseVertexIndex + 3].mPosition = cubePoints[7] + cubeOffset;
        }
    }

    // add indices
    int baseIndex = meshData.mBlocksIndices.size();
    meshData.mBlocksIndices.resize(baseIndex + 6);
    meshData.mBlocksIndices[baseIndex + 0] = baseVertexIndex + 3;
    meshData.mBlocksIndices[baseIndex + 1] = baseVertexIndex + 1;
    meshData.mBlocksIndices[baseIndex + 2] = baseVertexIndex + 0;
    meshData.mBlocksIndices[baseIndex + 3] = baseVertexIndex + 3;
    meshData.mBlocksIndices[baseIndex + 4] = baseVertexIndex + 2;
    meshData.mBlocksIndices[baseIndex + 5] = baseVertexIndex + 1;
}

float GameMapHelpers::GetSlopeHeight(int slope, float posx, float posy)
{
    debug_assert(posx >= 0.0f && posx <= 1.0f);
    debug_assert(posy >= 0.0f && posy <= 1.0f);

    float slopeMin = 0.0f;
    float slopeMax = 0.0f;
    float t = 0.0f;

    switch (slope)
    {
        case 0: return 0.0f;
        // N, 26 low, high
        case 1: case 2:
            slopeMin = ((slope - 1 + 1) / 2.0f) * MAP_BLOCK_LENGTH;
            slopeMax = ((slope - 1 + 0) / 2.0f) * MAP_BLOCK_LENGTH;
            t = posy;
        break;
        // S, 26 low, high
        case 3: case 4:
            slopeMin = ((slope - 3 + 0) / 2.0f) * MAP_BLOCK_LENGTH;
            slopeMax = ((slope - 3 + 1) / 2.0f) * MAP_BLOCK_LENGTH;
            t = posy;
        break;
        // W, 26 low, high
        case 5: case 6:
            slopeMin = ((slope - 5 + 1) / 2.0f) * MAP_BLOCK_LENGTH;
            slopeMax = ((slope - 5 + 0) / 2.0f) * MAP_BLOCK_LENGTH;
            t = posx;
        break;
        // E, 26 low, high
        case 7: case 8:
            slopeMin = ((slope - 7 + 0) / 2.0f) * MAP_BLOCK_LENGTH;
            slopeMax = ((slope - 7 + 1) / 2.0f) * MAP_BLOCK_LENGTH;
            t = posx;
        break;
        // N, 7 low - high
        case 9: case 10: case 11: case 12:
        case 13: case 14: case 15: case 16:
            slopeMin = ((slope - 9 + 1) / 8.0f) * MAP_BLOCK_LENGTH;
            slopeMax = ((slope - 9 + 0) / 8.0f) * MAP_BLOCK_LENGTH;
            t = posy;
        break;
        // S, 7 low - high
        case 17: case 18: case 19: case 20:
        case 21: case 22: case 23: case 24:
            slopeMin = ((slope - 17 + 0) / 8.0f) * MAP_BLOCK_LENGTH;
            slopeMax = ((slope - 17 + 1) / 8.0f) * MAP_BLOCK_LENGTH;
            t = posy;
        break;
        // W, 7 low - high
        case 25: case 26: case 27: case 28:
        case 29: case 30: case 31: case 32:
            slopeMin = ((slope - 25 + 1) / 8.0f) * MAP_BLOCK_LENGTH;
            slopeMax = ((slope - 25 + 0) / 8.0f) * MAP_BLOCK_LENGTH;
            t = posx;
        break;
        // E, 7 low - high
        case 33: case 34: case 35: case 36:
        case 37: case 38: case 39: case 40:
            slopeMin = ((slope - 33 + 0) / 8.0f) * MAP_BLOCK_LENGTH;
            slopeMax = ((slope - 33 + 1) / 8.0f) * MAP_BLOCK_LENGTH;
            t = posx;
        break;
        // 41 - 44 = 45 N,S,W,E
        case 41: 
            slopeMin = MAP_BLOCK_LENGTH;
            slopeMax = 0.0f;
            t = posy;
        break;
        case 42: 
            slopeMin = 0.0f;
            slopeMax = MAP_BLOCK_LENGTH;
            t = posy;
        break;
        case 43: 
            slopeMin = MAP_BLOCK_LENGTH;
            slopeMax = 0.0f;
            t = posx;
        break;
        case 44: 
            slopeMin = 0.0f;
            slopeMax = MAP_BLOCK_LENGTH;
            t = posx;
        break;

        default:
        {
            debug_assert(false);
            return 0.0f;
        }
    }
    // linear interpolate point
    return glm::lerp(slopeMin, slopeMax, t);
}
