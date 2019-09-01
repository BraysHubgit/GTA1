#include "stdafx.h"
#include "CarnageGame.h"
#include "RenderSystem.h"
#include "SpriteCache.h"
#include "ConsoleWindow.h"

CarnageGame gCarnageGame;

bool CarnageGame::Initialize()
{
    mTopDownCameraController.SetupInitial();
    mCityScape.LoadFromFile("NYC.CMP");

    if (!gSpriteCache.CreateBlocksSpritesheet() || !gSpriteCache.CreateObjectsSpritesheet())
    {
        debug_assert(false);
    }

    return true;
}

void CarnageGame::Deinit()
{
    mCityScape.Cleanup();
}

void CarnageGame::UpdateFrame(Timespan deltaTime)
{
    mTopDownCameraController.UpdateFrame(deltaTime);
}

void CarnageGame::InputEvent(KeyInputEvent& inputEvent)
{
    if (inputEvent.mConsumed)
        return;

    if (inputEvent.mKeycode == KEYCODE_TILDE && inputEvent.mPressed) // show debug console
    {
        gDebugConsoleWindow.mWindowShown = true;
        return;
    }
    if (inputEvent.mKeycode == KEYCODE_F3 && inputEvent.mPressed)
    {
        gRenderSystem.ReloadRenderPrograms();
        return;
    }

    mTopDownCameraController.InputEvent(inputEvent);
}

void CarnageGame::InputEvent(MouseButtonInputEvent& inputEvent)
{
    mTopDownCameraController.InputEvent(inputEvent);
}

void CarnageGame::InputEvent(MouseMovedInputEvent& inputEvent)
{
    mTopDownCameraController.InputEvent(inputEvent);
}

void CarnageGame::InputEvent(MouseScrollInputEvent& inputEvent)
{
    mTopDownCameraController.InputEvent(inputEvent);
}

void CarnageGame::InputEvent(KeyCharEvent& inputEvent)
{
}