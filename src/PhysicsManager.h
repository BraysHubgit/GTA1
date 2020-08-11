#pragma once

#include "PhysicsDefs.h"
#include "GameDefs.h"
#include "PhysicsComponents.h"

// this class manages physics and collision detections for map and objects
class PhysicsManager final: private b2ContactListener
{
public:
    PhysicsManager();

    bool Initialize();
    void Deinit();
    void UpdateFrame();

    // create pedestrian specific physical body
    // @param pedestrian: Reference ped
    // @param position: Coord in world
    // @param rotationAngle: Heading
    PedPhysicsComponent* CreatePhysicsComponent(Pedestrian* pedestrian, const glm::vec3& position, cxx::angle_t rotationAngle);

    // create car specific physical body
    // @param car: Reference car
    // @param position: Coord in world
    // @param rotationAngle: Heading
    // @param desc: Car class description
    CarPhysicsComponent* CreatePhysicsComponent(Vehicle* car, const glm::vec3& position, cxx::angle_t rotationAngle, CarStyle* desc);

    // free physics object
    // @param object: Object to destroy, pointer becomes invalid
    void DestroyPhysicsComponent(PedPhysicsComponent* object);
    void DestroyPhysicsComponent(CarPhysicsComponent* object);

    // query all physics objects that intersects with line
    // note that depth is ignored so pointA and pointB has only 2 components
    // @param pointA, pointB: Line of intersect points
    // @param aaboxCenter, aabboxExtents: AABBox area of intersections
    // @param outputResult: Output objects
    void QueryObjectsLinecast(const glm::vec2& pointA, const glm::vec2& pointB, PhysicsLinecastResult& outputResult) const;
    void QueryObjectsWithinBox(const glm::vec2& aaboxCenter, const glm::vec2& aabboxExtents, PhysicsQueryResult& outputResult) const;

private:
    // create level map body, used internally
    void CreateMapCollisionShape();

    // apply gravity forces and correct y coord for objects
    void FixedStepGravity();

    void ProcessSimulationStep(bool resetPreviousState);
    void ProcessInterpolation();

    // override b2ContactFilter
	void BeginContact(b2Contact* contact) override;
	void EndContact(b2Contact* contact) override;
	void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;
	void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;

    bool CollidePedVsPed(b2Contact* contact, PedPhysicsComponent* pedA, PedPhysicsComponent* pedB);
    bool HasCollisionPedestrianVsMap(int mapx, int mapz, float height) const;
    bool HasCollisionCarVsMap(b2Contact* contact, b2Fixture* fixtureCar, int mapx, int mapz) const;
    bool HasCollisionPedestrianVsCar(b2Contact* contact, b2Fixture* fixturePed, b2Fixture* fixtureCar);

    bool ProcessSensorContact(b2Contact* contact, bool onBegin);

    bool GetContactComponents(b2Contact* contact, PedPhysicsComponent*& pedPhysicsObject, CarPhysicsComponent*& carPhysicsObject) const;

private:
    b2Body* mMapCollisionShape;
    b2World* mPhysicsWorld;

    float mSimulationTimeAccumulator;

    // physics components pools
    cxx::object_pool<PedPhysicsComponent> mPedsBodiesPool;
    cxx::object_pool<CarPhysicsComponent> mCarsBodiesPool;

    cxx::intrusive_list<PedPhysicsComponent> mPedsBodiesList;
    cxx::intrusive_list<CarPhysicsComponent> mCarsBodiesList;
};

extern PhysicsManager gPhysics;