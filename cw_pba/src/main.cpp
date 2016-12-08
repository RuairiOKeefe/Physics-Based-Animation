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

unique_ptr<Entity> CreateBox(const vec3 &position)//May want to make more of these, perhaps make a factory?
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

	return ent;
}

bool load_content()
{
	phys::Init();
	for (size_t i = 0; i < 4; i++) {
		SceneList.push_back(move(CreateParticle()));
	}
	SceneList.push_back(move(CreateBox({ 0, 4, 0 })));

	auto b = SceneList[0]->GetComponents("RigidBody");
	if (b.size() == 1)//Check only 1 phys component
	{
		const auto p = static_cast<cRigidBody *>(b[0]);//Find said phys component
		p->AddLinearImpulse(vec3(-500.0f, 1000.0f, 0.0f));//turns out impulse does work but needs to be really large because this will be multiplied by delta time, but only act for a single frame (perhaps look into allowing a desired velocity to be entered?)
	}

	floorEnt = unique_ptr<Entity>(new Entity());
	floorEnt->AddComponent(unique_ptr<Component>(new cPlaneCollider()));
	floorEnt->SetName("Floor");
	phys::SetCameraPos(vec3(20.0f, 10.0f, 20.0f));
	phys::SetCameraTarget(vec3(0, 10.0f, 0));
	InitPhysics();
	return true;
}

void launchSphere()//currently cubes; why you ask? Getting this to make anything was hell and you don't want to know how long this minor change took. Seriously the stress required to restore this method to working order has taken years off my life, thanks Ben.
{
	free_camera cam = phys::GetCamera();//Player/User camera.
	vec3 pos = cam.get_position();//The current position of the camera.
	vec3 dir = normalize(vec3(cosf(cam.get_pitch()) * -sinf(cam.get_yaw()), sinf(cam.get_pitch()), -cosf(cam.get_yaw()) * cosf(cam.get_pitch())));//The forward direction of the camera.
	SceneList.push_back(move(CreateBox(pos)));
	auto b = SceneList[SceneList.size()-1]->getComponent<cRigidCube>();
	b->AddLinearImpulse(dir*0.5f);//Adds impulse in direction player is facing, scaled by a value. Value should be tweaked later to serve our purposes.
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

	int state = glfwGetMouseButton(renderer::get_window(), GLFW_MOUSE_BUTTON_1);
	glfwSetInputMode(renderer::get_window(), GLFW_STICKY_MOUSE_BUTTONS, 1);

	if (state == GLFW_PRESS && canClick == true)
	{
		cout << "Click" << endl;
		launchSphere();
		canClick = false;
	}
	else
		if (state == GLFW_RELEASE)
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