#include "game.h"
#include "physics.h"
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <graphics_framework.h>
#include <phys_utils.h>
#include <thread>

using namespace std;
using namespace graphics_framework;
using namespace glm;
#define physics_tick 1.0 / 60.0

static vector<unique_ptr<Entity>> SceneList;
static unique_ptr<Entity> floorEnt;
bool canClick = true;

unique_ptr<Entity> CreateParticle() {
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(vec3(0, 5.0 + (double)(rand() % 200) / 20.0, 0));
	unique_ptr<Component> physComponent(new cPhysics());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::SPHERE));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->AddComponent(unique_ptr<Component>(new cSphereCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));
	
	return ent;
}

bool load_content()
{
	phys::Init();
	for (size_t i = 0; i < 5; i++)
	{
		SceneList.push_back(move(CreateParticle()));
	}
	SceneList[0]->SetPosition(vec3(-6.0f, 10.0f, -4.0f));
	auto b = SceneList[0]->GetComponents("Physics");
	if (b.size() == 1)//Check only 1 phys component
	{
		const auto p = static_cast<cPhysics *>(b[0]);//Find said phys component
		p->AddImpulse(vec3(-500.0f, 1000.0f, 0.0f));//turns out impulse does work but needs to be really large because this will be multiplied by delta time, but only act for a single frame (perhaps look into allowing a desired velocity to be entered?)
	}
	//floorEnt = unique_ptr<Entity>(new Entity());
	//floorEnt->AddComponent(unique_ptr<Component>(new cPlaneCollider()));
	phys::SetCameraPos(vec3(20.0f, 10.0f, 20.0f));
	phys::SetCameraTarget(vec3(0, 10.0f, 0));
	InitPhysics();
	return true;
}

void launchSphere()//currently particles, but baby steps and all that
{
	free_camera cam = phys::GetCamera();
	vec3 pos = cam.get_position();
	vec3 dir = normalize(vec3(cosf(cam.get_pitch()) * -sinf(cam.get_yaw()), sinf(cam.get_pitch()), -cosf(cam.get_yaw()) * cosf(cam.get_pitch())));

	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(pos);
	unique_ptr<Component> physComponent(new cPhysics());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::SPHERE));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->AddComponent(unique_ptr<Component>(new cSphereCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));

	auto b = ent->GetComponents("Physics");
	if (b.size() == 1)
	{
		const auto p = static_cast<cPhysics *>(b[0]);
		p->AddImpulse(dir*5000.0f);
	}

	SceneList.push_back(move(ent));
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