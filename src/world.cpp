#include "world.hpp"

void GameWorld::initPhysx()
{
	m_physxHandle.foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_physxHandle.allocator, m_physxHandle.errorCallback);
	m_physxHandle.physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_physxHandle.foundation, physx::PxTolerancesScale());
	m_physxHandle.dispatcher = physx::PxDefaultCpuDispatcherCreate(1);
}

GameWorld::GameWorld() { initPhysx(); }
GameWorld::~GameWorld()
{
	m_physxHandle.dispatcher->release();
	m_physxHandle.physics->release();
	m_physxHandle.foundation->release();
}

void GameWorld::update(float deltatime)
{
	m_scenes[m_activeScene]->update(deltatime);
}

void GameWorld::render()
{
	m_scenes[m_activeScene]->render();
}

void GameWorld::removeScene(uint32_t id)
{
	m_scenes.erase(id);
}

void GameWorld::mouseCallback(roj::MouseButton button, roj::InputAction action)
{
	m_scenes[m_activeScene]->mouseCallback(button, action);
}

void GameWorld::keyCallback(roj::Keycode key, roj::InputAction action)
{
	m_scenes[m_activeScene]->keyCallback(key, action);

}

void GameWorld::cursorCallback(double x, double y)
{
	m_scenes[m_activeScene]->cursorCallback(x, y);

}

void GameWorld::setActiveScene(uint32_t scene)
{
	m_activeScene = scene;
}