#include "game.h"
#include "physics.h"
#include "cPhysicsComponents.h"
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <graphics_framework.h>
#include <phys_utils.h>
#include <thread>
#include "main.h"

using namespace std;
using namespace graphics_framework;
using namespace glm;
#define physics_tick 1.0 / 60.0

vector<unique_ptr<Entity>> SceneList;
unique_ptr<Entity> floorEnt;
bool canClick = true;
bool canSwitch = true;
enum shape {cube, sphere};
shape launchType = cube;

unique_ptr<Entity> CreateParticle() {
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(vec3(-2.0, 5.0 + (double)(rand() % 200) / 20.0, 2.0));
	unique_ptr<Component> physComponent(new cParticle());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::SPHERE));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->AddComponent(unique_ptr<Component>(new cSphereCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));
	return ent;
}

unique_ptr<Entity> CreateBox(const vec3 &position)
{
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(position);
	ent->SetRotation(angleAxis(0.0f, vec3(1, 0, 0)));
	unique_ptr<Component> physComponent(new cRigidCube());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::BOX));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->SetName("Cube");
	ent->AddComponent(unique_ptr<Component>(new cBoxCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));

	return ent;
}

unique_ptr<Entity> CreateBox(const vec3 &position, quat &rotation)
{
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(position);
	ent->SetRotation(rotation);
	unique_ptr<Component> physComponent(new cRigidCube());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::BOX));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->SetName("Cube");
	ent->AddComponent(unique_ptr<Component>(new cBoxCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));

	return ent;
}

unique_ptr<Entity> CreateBox(const vec3 &position, dvec3 &springPos, double restLength)
{
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(position);
	ent->SetRotation(angleAxis(-45.0f, vec3(1, 0, 0)));
	unique_ptr<Component> physComponent(new cRigidCube());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::BOX));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->SetName("Cube");
	ent->AddComponent(unique_ptr<Component>(new cBoxCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));
	ent->getComponent<cRigidCube>()->mass = 1.0;
	ent->getComponent<cRigidCube>()->hasSpring = true;
	ent->getComponent<cRigidCube>()->springPos = springPos;
	ent->getComponent<cRigidCube>()->restLength = restLength;
	ent->getComponent<cRigidCube>()->elasticity = 5;

	return ent;
}

unique_ptr<Entity> CreateSphere(const vec3 &position) {
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(position);
	ent->SetRotation(angleAxis(0.0f, vec3(1, 0, 0)));
	unique_ptr<Component> physComponent(new cRigidSphere());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::SPHERE));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->AddComponent(unique_ptr<Component>(new cSphereCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));
	return ent;
}

bool load_content()
{
	glfwSetInputMode(renderer::get_window(), GLFW_STICKY_MOUSE_BUTTONS, 1);
	phys::Init();
	for (size_t i = 0; i < 4; i++) {
		SceneList.push_back(move(CreateParticle()));
	}
	SceneList.push_back(move(CreateBox({ 0, 4, 0 })));
	//SceneList.push_back(move(CreateBox({ 0, 6, 0 })));
	//SceneList.push_back(move(CreateBox({ 0, 8, 0 })));
	floorEnt = unique_ptr<Entity>(new Entity());
	floorEnt->AddComponent(unique_ptr<Component>(new cPlaneCollider()));
	floorEnt->SetName("Floor");
	phys::SetCameraPos(vec3(20.0f, 10.0f, 20.0f));
	phys::SetCameraTarget(vec3(0, 10.0f, 0));
	InitPhysics();
	return true;
}

void launchCube()
{
	free_camera cam = phys::GetCamera();//Player/User camera.
	dvec3 pos = cam.get_position();//The current position of the camera.
	dvec3 dir = normalize(vec3(cosf(cam.get_pitch()) * -sinf(cam.get_yaw()), sinf(cam.get_pitch()), -cosf(cam.get_yaw()) * cosf(cam.get_pitch())));//The forward direction of the camera.
	SceneList.push_back(move(CreateBox(pos, quat(dvec4(dir, 1.0)))));
	auto b = SceneList[SceneList.size() - 1]->getComponent<cRigidCube>();
	b->AddLinearImpulse(dir*1.0);//Adds impulse in direction player is facing, scaled by a value. Value should be tweaked later to serve our purposes.
}

void launchSphere()
{
	free_camera cam = phys::GetCamera();//Player/User camera.
	dvec3 pos = cam.get_position();//The current position of the camera.
	dvec3 dir = normalize(vec3(cosf(cam.get_pitch()) * -sinf(cam.get_yaw()), sinf(cam.get_pitch()), -cosf(cam.get_yaw()) * cosf(cam.get_pitch())));//The forward direction of the camera.
	SceneList.push_back(move(CreateSphere(pos)));
	auto b = SceneList[SceneList.size()-1]->getComponent<cRigidSphere>();
	b->AddLinearImpulse(dir*1.0);//Adds impulse in direction player is facing, scaled by a value. Value should be tweaked later to serve our purposes.
}

bool update(double delta_time) {
	static double t = 0.0;
	static double accumulator = 0.0;
	accumulator += delta_time;

	while (accumulator > physics_tick) {
		UpdatePhysics(t, physics_tick);
		accumulator -= physics_tick;
		t += physics_tick;
	}

	int mouseState = glfwGetMouseButton(renderer::get_window(), GLFW_MOUSE_BUTTON_1);
	int switchState = glfwGetKey(renderer::get_window(), GLFW_KEY_E);

	if (switchState == GLFW_PRESS && canSwitch == true)
	{
		if (launchType == cube)
		{
			launchType = sphere;
			cout << "switched to sphere" << endl;
		}
		else
		{
			if (launchType == sphere)
			{
				launchType = cube;
				cout << "switched to cube" << endl;
			}
		}
		canSwitch = false;
	}
	else
		if (switchState == GLFW_RELEASE)
		{
			canSwitch = true;
		}

	if (mouseState == GLFW_PRESS && canClick == true)
	{
		switch (launchType)
		{
			case cube:
				launchCube();
				break;
			case sphere:
				launchSphere();
				break;
		}
		canClick = false;
	}
	else
		if (mouseState == GLFW_RELEASE)
		{
			canClick = true;
		}

	for (auto &e : SceneList) {
		e->Update(delta_time);
	}
	phys::Update(delta_time);
	return true;
}

bool render() {
	for (auto &e : SceneList) {
		e->Render();
	}
	phys::DrawScene();
	return true;
}

void main() {
	// Create application
	app application;
	// Set load content, update and render methods
	application.set_load_content(load_content);
	application.set_update(update);
	application.set_render(render);
	// Run application
	application.run();
}