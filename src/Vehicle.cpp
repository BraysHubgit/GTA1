#include "stdafx.h"
#include "Vehicle.h"
#include "PhysicsManager.h"
#include "PhysicsComponents.h"
#include "GameMapManager.h"
#include "SpriteBatch.h"
#include "RenderingManager.h"
#include "SpriteManager.h"
#include "Pedestrian.h"
#include "TimeManager.h"

Vehicle::Vehicle(GameObjectID id) : GameObject(eGameObjectClass_Car, id)
    , mPhysicsBody()
    , mDead()
    , mCarStyle()
    , mDamageDeltaBits()
    , mDrawHeight()
    , mRemapIndex(NO_REMAP)
{
}

Vehicle::~Vehicle()
{
    // todo: remove all passengers
    
    if (mPhysicsBody)
    {
        gPhysics.DestroyPhysicsBody(mPhysicsBody);
    }
    gSpriteManager.FlushSpritesCache(mObjectID);
}

void Vehicle::Spawn(const glm::vec3& startPosition, cxx::angle_t startRotation)
{
    debug_assert(mCarStyle);
    
    if (mPhysicsBody == nullptr)
    {
        mPhysicsBody = gPhysics.CreatePhysicsBody(this, startPosition, startRotation);
        debug_assert(mPhysicsBody);
    }
    else
    {   
        mPhysicsBody->SetPosition(startPosition, startRotation);
    }

    mDead = false;
    mDamageDeltaBits = 0;
    mChassisSpriteIndex = gGameMap.mStyleData.GetCarSpriteIndex(mCarStyle->mVType, mCarStyle->mSprNum); // todo: handle bike fallen state 

    SetupDeltaAnimations();
}

void Vehicle::UpdateFrame()
{
    UpdateDeltaAnimations();

    UpdateDriving();
}

void Vehicle::PreDrawFrame()
{   
    // sync sprite transformation with physical body
    cxx::angle_t rotationAngle = mPhysicsBody->GetRotationAngle() - cxx::angle_t::from_degrees(SPRITE_ZERO_ANGLE);
    glm::vec3 position = mPhysicsBody->mSmoothPosition;
    ComputeDrawHeight(position);

    int remapClut = mRemapIndex == NO_REMAP ? 0 : (mCarStyle->mRemapsBaseIndex + mRemapIndex);
    gSpriteManager.GetSpriteTexture(mObjectID, mChassisSpriteIndex, remapClut, GetSpriteDeltas(), mDrawSprite);

    mDrawSprite.mPosition.x = position.x;
    mDrawSprite.mPosition.y = position.z;
    mDrawSprite.mRotateAngle = rotationAngle;
    mDrawSprite.mHeight = mDrawHeight;
    mDrawSprite.SetOriginToCenter();
}

void Vehicle::DrawDebug(DebugRenderer& debugRender)
{
    glm::vec3 position = mPhysicsBody->GetPosition();

    glm::vec2 corners[4];
    mPhysicsBody->GetChassisCorners(corners);

    glm::vec3 points[4];
    for (int i = 0; i < 4; ++i)
    {
        points[i].x = corners[i].x;
        points[i].z = corners[i].y;
        points[i].y = mDrawHeight;
    }
    for (int i = 0; i < 4; ++i)
    {
        debugRender.DrawLine(points[i], points[(i + 1) % 4], Color32_Red, false);
    }

    // draw doors
    for (int idoor = 0; idoor < mCarStyle->mDoorsCount; ++idoor)
    {
        glm::vec2 rotated_pos;
        GetDoorPos(idoor, rotated_pos);
        debugRender.DrawCube(glm::vec3(rotated_pos.x, position.y + 0.15f, rotated_pos.y), glm::vec3(0.15f, 0.15f, 0.15f), Color32_Yellow, false);
    }

    if (mCarStyle->mDoorsCount > 0)
    {
        glm::vec2 seatpos;
        GetSeatPos(eCarSeat_Driver, seatpos);
        debugRender.DrawCube(glm::vec3(seatpos.x, position.y + 0.15f, seatpos.y), glm::vec3(0.15f, 0.15f, 0.15f), Color32_Green, false);
    }

    // draw body velocity
    glm::vec2 bodyLinearVelocity = mPhysicsBody->GetLinearVelocity();
    debugRender.DrawLine(
        glm::vec3 {position.x, mDrawHeight, position.z},
        glm::vec3 {position.x + bodyLinearVelocity.x, mDrawHeight, position.z + bodyLinearVelocity.y}, Color32_Cyan, false);

    // draw wheels
    for (eCarWheel currID: {eCarWheel_Drive, eCarWheel_Steer})
    {
        mPhysicsBody->GetWheelCorners(currID, corners);

        for (int i = 0; i < 4; ++i)
        {
            points[i].x = corners[i].x;
            points[i].z = corners[i].y;
            points[i].y = mDrawHeight;
        }
        for (int i = 0; i < 4; ++i)
        {
            debugRender.DrawLine(points[i], points[(i + 1) % 4], Color32_Yellow, false);
        }
        glm::vec2 forwardVelocity = mPhysicsBody->GetWheelForwardVelocity(currID);
        glm::vec2 lateralVelocity = mPhysicsBody->GetWheelLateralVelocity(currID);
        glm::vec2 wheelPosition = mPhysicsBody->GetWheelPosition(currID);

        debugRender.DrawLine(
            glm::vec3 {wheelPosition.x, mDrawHeight, wheelPosition.y},
            glm::vec3 {wheelPosition.x + forwardVelocity.x, mDrawHeight, wheelPosition.y + forwardVelocity.y}, Color32_Green, false);
        debugRender.DrawLine(
            glm::vec3 {wheelPosition.x, mDrawHeight, wheelPosition.y},
            glm::vec3 {wheelPosition.x + lateralVelocity.x, mDrawHeight, wheelPosition.y + lateralVelocity.y}, Color32_Red, false);

        glm::vec2 signDirection = mPhysicsBody->GetWheelDirection(currID);
        debugRender.DrawLine(
            glm::vec3 {wheelPosition.x, mDrawHeight, wheelPosition.y},
            glm::vec3 {wheelPosition.x + signDirection.x, mDrawHeight, wheelPosition.y + signDirection.y}, Color32_Yellow, false);
    }
}

void Vehicle::ComputeDrawHeight(const glm::vec3& position)
{
    glm::vec2 corners[4];
    mPhysicsBody->GetChassisCorners(corners);

    float maxHeight = position.y;

    for (int icorner = 0; icorner < 4; ++icorner)
    {
        glm::vec3 cornerPosition { corners[icorner].x, position.y, corners[icorner].y };
        float cornerHeight = gGameMap.GetHeightAtPosition(cornerPosition);
        if (cornerHeight > maxHeight)
        {
            maxHeight = cornerHeight;
        }
    }

    mDrawHeight = maxHeight + 0.02f; // todo: magic numbers
}

SpriteDeltaBits Vehicle::GetSpriteDeltas() const
{
    SpriteDeltaBits deltaBits = mDamageDeltaBits;

    // add doors
    for (int idoor = 0; idoor < MAX_CAR_DOORS; ++idoor)
    {
        if (!mDoorsAnims[idoor].IsNull())
        {
            unsigned int deltaBit = mDoorsAnims[idoor].GetCurrentFrame();
            deltaBits |= deltaBit;
        }
    }

    // add emergency lights
    if (mEmergLightsAnim.IsAnimationActive())
    {
        unsigned int deltaBit = mEmergLightsAnim.GetCurrentFrame();
        deltaBits |= deltaBit;
    }

    return deltaBits;
}

void Vehicle::SetupDeltaAnimations()
{
    SpriteDeltaBits deltaBits = gGameMap.mStyleData.mSprites[mChassisSpriteIndex].GetDeltaBits();

    mEmergLightsAnim.Clear();
    for (int idoor = 0; idoor < MAX_CAR_DOORS; ++idoor)
    {
        mDoorsAnims[idoor].Clear();
    }

    SpriteDeltaBits maskBits = BIT(CAR_LIGHTING_SPRITE_DELTA_0) | BIT(CAR_LIGHTING_SPRITE_DELTA_1);
    if ((deltaBits & maskBits) == maskBits)
    {
        mEmergLightsAnim.Clear();
        mEmergLightsAnim.mAnimDesc.SetupFrames(
        {
            BIT(CAR_LIGHTING_SPRITE_DELTA_0), BIT(CAR_LIGHTING_SPRITE_DELTA_0), BIT(CAR_LIGHTING_SPRITE_DELTA_0),
            BIT(CAR_LIGHTING_SPRITE_DELTA_1), BIT(CAR_LIGHTING_SPRITE_DELTA_1), BIT(CAR_LIGHTING_SPRITE_DELTA_1),
        }, 
        CAR_DELTA_ANIMS_SPEED);
    }

    // doors
    if (mCarStyle->mDoorsCount >= 1)
    {
        mDoorsAnims[0].mAnimDesc.SetupFrames(
        {
            0,
            BIT(CAR_DOOR1_SPRITE_DELTA_0),
            BIT(CAR_DOOR1_SPRITE_DELTA_1),
            BIT(CAR_DOOR1_SPRITE_DELTA_2),
            BIT(CAR_DOOR1_SPRITE_DELTA_3)
        }, 
        CAR_DELTA_ANIMS_SPEED);
    }

    if (mCarStyle->mDoorsCount >= 2)
    {
        mDoorsAnims[1].mAnimDesc.SetupFrames(
        {
            0,
            BIT(CAR_DOOR2_SPRITE_DELTA_0),
            BIT(CAR_DOOR2_SPRITE_DELTA_1),
            BIT(CAR_DOOR2_SPRITE_DELTA_2),
            BIT(CAR_DOOR2_SPRITE_DELTA_3)
        }, 
        CAR_DELTA_ANIMS_SPEED);
    }
}

void Vehicle::UpdateDeltaAnimations()
{
    float deltaTime = gTimeManager.mGameFrameDelta;
    for (int idoor = 0; idoor < MAX_CAR_DOORS; ++idoor)
    {
        if (mDoorsAnims[idoor].IsAnimationActive())
        {
            mDoorsAnims[idoor].AdvanceAnimation(deltaTime);
        }
    }

    if (mEmergLightsAnim.IsAnimationActive())
    {
        mEmergLightsAnim.AdvanceAnimation(deltaTime);
    }
}

void Vehicle::OpenDoor(int doorIndex)
{
    if (HasDoorAnimation(doorIndex))
    {
        if (IsDoorOpening(doorIndex) || IsDoorOpened(doorIndex))
            return;

        mDoorsAnims[doorIndex].PlayAnimation(eSpriteAnimLoop_None);
    }
}

void Vehicle::CloseDoor(int doorIndex)
{
    if (HasDoorAnimation(doorIndex))
    {
        if (IsDoorClosing(doorIndex) || IsDoorClosed(doorIndex))
            return;

        mDoorsAnims[doorIndex].PlayAnimationBackwards(eSpriteAnimLoop_None);
    }
}

bool Vehicle::HasDoorAnimation(int doorIndex) const
{
    debug_assert(doorIndex < MAX_CAR_DOORS);
    return !mDoorsAnims[doorIndex].IsNull();
}

bool Vehicle::IsDoorOpened(int doorIndex) const
{
    debug_assert(doorIndex < MAX_CAR_DOORS);
    return HasDoorAnimation(doorIndex) && !mDoorsAnims[doorIndex].IsAnimationActive() && 
        mDoorsAnims[doorIndex].IsLastFrame();
}

bool Vehicle::IsDoorClosed(int doorIndex) const
{
    debug_assert(doorIndex < MAX_CAR_DOORS);
    return HasDoorAnimation(doorIndex) && !mDoorsAnims[doorIndex].IsAnimationActive() && 
        mDoorsAnims[doorIndex].IsFirstFrame();
}

bool Vehicle::IsDoorOpening(int doorIndex) const
{
    debug_assert(doorIndex < MAX_CAR_DOORS);
    return HasDoorAnimation(doorIndex) && mDoorsAnims[doorIndex].IsAnimationActive() && 
        mDoorsAnims[doorIndex].IsRunsForwards();
}

bool Vehicle::IsDoorClosing(int doorIndex) const
{
    debug_assert(doorIndex < MAX_CAR_DOORS);
    return HasDoorAnimation(doorIndex) && mDoorsAnims[doorIndex].IsAnimationActive() && 
        mDoorsAnims[doorIndex].IsRunsBackwards();
}

bool Vehicle::HasEmergencyLightsAnimation() const
{
    return !mEmergLightsAnim.IsNull();
}

bool Vehicle::IsEmergencyLightsEnabled() const
{
    return HasEmergencyLightsAnimation() && mEmergLightsAnim.IsAnimationActive();
}

void Vehicle::EnableEmergencyLights(bool isEnabled)
{
    if (HasEmergencyLightsAnimation())
    {
        if (isEnabled == mEmergLightsAnim.IsAnimationActive())
            return;

        if (isEnabled)
        {
            mEmergLightsAnim.PlayAnimation(eSpriteAnimLoop_FromStart);
        }
        else
        {
            mEmergLightsAnim.StopAnimation();
            mEmergLightsAnim.RewindToStart();
        }
    }
}

int Vehicle::GetDoorIndexForSeat(eCarSeat carSeat) const
{
    switch (carSeat)
    {
        case eCarSeat_Driver: return 0;
        case eCarSeat_Passenger: return 1;
        case eCarSeat_PassengerExtra: return 2;
    }
    debug_assert(false);
    return 0;
}

bool Vehicle::GetDoorPosLocal(int doorIndex, glm::vec2& out) const
{
    if (doorIndex > -1 && doorIndex < mCarStyle->mDoorsCount)
    {
        out.x = Convert::PixelsToMeters(mCarStyle->mDoors[doorIndex].mRpy);
        out.y = -Convert::PixelsToMeters(mCarStyle->mDoors[doorIndex].mRpx);
        return true;
    }

    debug_assert(false);
    return false;
}

bool Vehicle::GetDoorPos(int doorIndex, glm::vec2& out) const
{
    if (GetDoorPosLocal(doorIndex, out))
    {
        out = mPhysicsBody->GetWorldPoint(out);
        return true;
    }

    debug_assert(false);
    return false;
}

bool Vehicle::GetSeatPosLocal(eCarSeat carSeat, glm::vec2& out) const
{
    int doorIndex = GetDoorIndexForSeat(carSeat);

    glm::vec2 doorLocalPos;
    if (GetDoorPosLocal(doorIndex, doorLocalPos))
    {
        bool isBike = (mCarStyle->mVType == eCarVType_Motorcycle);
        if (isBike)
        {
            out.y = 0.0f; // dead center
        }
        else
        {
            out.y = -Convert::PixelsToMeters(mCarStyle->mWidth) * 0.25f * -glm::sign(doorLocalPos.y);
        }
        out.x = doorLocalPos.x;
        return true;
    }
    return false;
}

bool Vehicle::GetSeatPos(eCarSeat carSeat, glm::vec2& out) const
{
    if (GetSeatPosLocal(carSeat, out))
    {
        out = mPhysicsBody->GetWorldPoint(out);
        return true;
    }

    debug_assert(false);
    return false;
}

bool Vehicle::IsSeatPresent(eCarSeat carSeat) const
{
    int doorIndex = GetDoorIndexForSeat(carSeat);
    return (doorIndex < mCarStyle->mDoorsCount && doorIndex > -1);
}

void Vehicle::PutPassenger(Pedestrian* pedestrian, eCarSeat carSeat)
{
    if (pedestrian == nullptr || carSeat == eCarSeat_Any)
    {
        debug_assert(false);
        return;
    }

    if (pedestrian->IsAttachedToObject(this)) // already attached
    {
        debug_assert(false);
        return;
    }

    pedestrian->SetAttachedToObject(this);
    mPassengers.push_back(pedestrian);
}

void Vehicle::RemovePassenger(Pedestrian* pedestrian)
{
    if (pedestrian->IsAttachedToObject(this))
    {
        pedestrian->SetDetached();
        cxx::erase_elements(mPassengers, pedestrian);
    }
    else
    {
        debug_assert(false);
    }
}

Pedestrian* Vehicle::GetCarDriver() const
{
    for (Pedestrian* currPassenger: mPassengers)
    {
        if (currPassenger->mCurrentSeat == eCarSeat_Driver)
            return currPassenger;
    }
    return nullptr;
}

Pedestrian* Vehicle::GetFirstPassenger(eCarSeat carSeat) const
{
    for (Pedestrian* currPassenger: mPassengers)
    {
        if (currPassenger->mCurrentSeat == carSeat)
            return currPassenger;
    }
    return nullptr;
}

void Vehicle::UpdateDriving()
{
    Pedestrian* carDriver = GetCarDriver();
    if (carDriver == nullptr || ePedestrianState_DrivingCar != carDriver->GetCurrentStateID())
    {
        mPhysicsBody->ResetDriveState();
        return;
    }

    mPhysicsBody->SetHandBrake(carDriver->mCtlActions[ePedestrianAction_HandBrake]);
    mPhysicsBody->SetAcceleration(carDriver->mCtlActions[ePedestrianAction_Accelerate]);
    mPhysicsBody->SetDeceleration(carDriver->mCtlActions[ePedestrianAction_Reverse]);

    // steering
    int currentSteering = CarSteeringDirectionNone;
    if (carDriver->mCtlActions[ePedestrianAction_SteerLeft])
    {
        currentSteering = CarSteeringDirectionLeft;
    }
    if (carDriver->mCtlActions[ePedestrianAction_SteerRight])
    {
        currentSteering = CarSteeringDirectionRight;
    }
    mPhysicsBody->SetSteering(currentSteering);
}

void Vehicle::ReceiveDamageFromWater()
{
    mDead = true;

    // kill passengers inside
    for (Pedestrian* currentPed: mPassengers)
    {
        currentPed->Die(ePedestrianDeathReason_Drowned, nullptr);
    }
}

bool Vehicle::HasHardTop() const
{
    if ((mCarStyle->mConvertible == eCarConvertible_HardTop || mCarStyle->mConvertible == eCarConvertible_HardTopAnimated) && 
        (mCarStyle->mVType != eCarVType_Motorcycle))
    {
        return true;
    }
    return false;
}
