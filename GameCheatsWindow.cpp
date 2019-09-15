#include "stdafx.h"
#include "GameCheatsWindow.h"
#include "imgui.h"
#include "RenderingManager.h"
#include "PhysicsManager.h"

GameCheatsWindow gGameCheatsWindow;

GameCheatsWindow::GameCheatsWindow()
    : DebugWindow("Game Cheats")
    , mGenerateFullMeshForMap()
    , mDrawPhysicsDebugShapes()
{
    for (int ilayer = 0; ilayer < MAP_LAYERS_COUNT; ++ilayer)
    {
        mDrawMapLayers[ilayer] = true;
    }
}

void GameCheatsWindow::DoUI(Timespan deltaTime)
{
    ImGuiWindowFlags wndFlags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | 
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize;

    ImGuiIO& io = ImGui::GetIO();
    if (!ImGui::Begin(mWindowName, &mWindowShown, wndFlags))
    {
        ImGui::End();
        return;
    }
    
    // pedestrian stats
    if (Pedestrian* pedestrian = gCarnageGame.mPlayerPedestrian)
    {
        ImGui::Separator();
        glm::vec3 pedPosition = pedestrian->mPhysicalBody->GetPosition();
        ImGui::Text("pos: %f, %f, %f", pedPosition.x, pedPosition.y, pedPosition.z);

        float pedHeading = pedestrian->mPhysicalBody->GetAngleDegrees();
        ImGui::Text("heading: %f", pedHeading);
        ImGui::Separator();

        MapCoord curr_block = pedPosition;

        BlockStyleData* currBlock = gGameMap.GetBlockClamp(curr_block);
        debug_assert(currBlock);

        ImGui::Text("b ground: %s", cxx::enum_to_string(currBlock->mGroundType));
        ImGui::Text("b slope: %d", currBlock->mSlopeType);
        ImGui::Text("b directions: %d, %d, %d, %d", currBlock->mUpDirection, currBlock->mRightDirection, 
            currBlock->mDownDirection, currBlock->mLeftDirection);
        ImGui::Text("b flat: %d", currBlock->mIsFlat);

        // draw debug block

        glm::vec3 cube_center { 
            MAP_BLOCK_LENGTH * curr_block.x + MAP_BLOCK_LENGTH * 0.5f,
            MAP_BLOCK_LENGTH * curr_block.z + MAP_BLOCK_LENGTH * 0.5f + 0.05f,
            MAP_BLOCK_LENGTH * curr_block.y + MAP_BLOCK_LENGTH * 0.5f,
        };
        glm::vec3 cube_dims {
            MAP_BLOCK_LENGTH - 0.1f,
            MAP_BLOCK_LENGTH - 0.1f,
            MAP_BLOCK_LENGTH - 0.1f
        };  

        gRenderManager.mDebugRenderer.DrawCube(cube_center, cube_dims, COLOR_GREEN);
        ImGui::Separator();
    }

    if (ImGui::Checkbox("Generate full mesh for map", &mGenerateFullMeshForMap))
    {
        gRenderManager.mCityRenderer.InvalidateMapMesh();
    }

    ImGui::Separator();

    const char* modeStrings[] = { "Follow", "Free Look" };
    CameraController* modeControllers[] =
    {
        &gCarnageGame.mFollowCameraController, 
        &gCarnageGame.mFreeLookCameraController,
    }; 
    int currentCameraMode = 0;
    for (int i = 0; i < IM_ARRAYSIZE(modeControllers); ++i)
    {
        if (gCarnageGame.mCameraController == modeControllers[i])
        {
            currentCameraMode = i;
            break;
        }
    }
    const char* item_current = modeStrings[currentCameraMode];
    if (ImGui::BeginCombo("Camera mode", item_current))
    {
        for (int n = 0; n < IM_ARRAYSIZE(modeStrings); n++)
        {
            bool is_selected = (item_current == modeStrings[n]);
            if (ImGui::Selectable(modeStrings[n], is_selected))
            {
                item_current = modeStrings[n];
                gCarnageGame.SetCameraController(modeControllers[n]);
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::CollapsingHeader("Physics"))
    {
        if (ImGui::Checkbox("Draw physics debug shapes", &mDrawPhysicsDebugShapes))
        {
            gPhysics.EnableDebugDraw(mDrawPhysicsDebugShapes);
        }
        ImGui::Separator();
    }

    if (ImGui::CollapsingHeader("Map layers"))
    {
        for (int ilayer = 0; ilayer < MAP_LAYERS_COUNT; ++ilayer)
        {
            cxx::string_buffer_16 cbtext;
            cbtext.printf("layer %d", ilayer);
            ImGui::Checkbox(cbtext.c_str(), &mDrawMapLayers[ilayer]);
        }
    }

    if (ImGui::CollapsingHeader("Ped"))
    {
        ImGui::SliderFloat("Turn speed", &gGameRules.mPedestrianTurnSpeed, 10.0f, 640.0f, "%.2f");
        ImGui::SliderFloat("Run speed", &gGameRules.mPedestrianRunSpeed, 0.1f, 16.0f, "%.2f");
        ImGui::SliderFloat("Walk speed", &gGameRules.mPedestrianWalkSpeed, 0.1f, 16.0f, "%.2f");
    }

    ImGui::End();
}