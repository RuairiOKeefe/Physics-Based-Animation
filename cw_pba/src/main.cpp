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

free_camera cam;
double cursor_x = 0.0;
double cursor_y = 0.0;
float speed;

static vector<unique_ptr<Entity>> SceneList;
static unique_ptr<Entity> floorEnt;

unique_ptr<Entity> CreateParticle() {
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(vec3(0, 5.0 + (double)(rand() % 200) / 20.0, 0));
	unique_ptr<Component> physComponent(new cPhysics());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::SPHERE));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->AddComponent(unique_ptr<Component>(new cSphereCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));
	mesh arsehole = mesh(geometry_builder::create_sphere(20, 20));
	unique_ptr<Component> meshComponent(new mesh(geometry_builder::create_sphere(20, 20)));//find a way to add a mesh component

	
	return ent;
}

bool initialise()
{
	glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwGetCursorPos(renderer::get_window(), &cursor_x, &cursor_y);
	return true;
}

bool load_content()
{
	cam.set_position(vec3(0.0f, 0.0f, 50.0f));
	cam.set_target(vec3(0.0f, 0.0f, 0.0f));
	auto aspect = static_cast<float>(renderer::get_screen_width()) / static_cast<float>(renderer::get_screen_height());
	cam.set_projection(quarter_pi<float>(), aspect, 0.4f, 1500.0f);
	phys::Init();
	for (size_t i = 0; i < 1; i++) {
		SceneList.push_back(move(CreateParticle()));
	}
	SceneList[0]->SetPosition(vec3(-6.0f, 10.0f, -4.0f));
	auto b = SceneList[0]->GetComponents("Physics");
	if (b.size() == 1)//Check only 1 phys component
	{
		const auto p = static_cast<cPhysics *>(b[0]);//Find said phys component
		p->AddImpulse(vec3(-500.0f, 1000.0f, 0.0f));//turns out impulse does work but needs to be really large because this will be multiplied by delta time, but only act for a single frame (perhaps look into allowing a desired velocity to be entered?)
	}
	floorEnt = unique_ptr<Entity>(new Entity());
	floorEnt->AddComponent(unique_ptr<Component>(new cPlaneCollider()));
	phys::SetCameraPos(vec3(20.0f, 10.0f, 20.0f));
	phys::SetCameraTarget(vec3(0, 10.0f, 0));
	InitPhysics();
	return true;
}

void FreeCam(float delta_time)
{
	static double ratio_width = quarter_pi<float>() / static_cast<float>(renderer::get_screen_width());
	static double ratio_height = (quarter_pi<float>() * (static_cast<float>(renderer::get_screen_height()) / static_cast<float>(renderer::get_screen_width()))) / static_cast<float>(renderer::get_screen_height());

	double current_x;
	double current_y;

	glfwGetCursorPos(renderer::get_window(), &current_x, &current_y);

	double delta_x = current_x - cursor_x;
	double delta_y = current_y - cursor_y;

	delta_x *= ratio_width;
	delta_y *= ratio_height;

	cam.rotate(delta_x, -delta_y);

	vec3 pos;
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_LEFT_SHIFT))
	{
		speed = 5;
	}
	else
	{
		speed = 1;
	}
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_W))
		pos += vec3(0.0f, 0.0f, 5.0f) * delta_time * speed;
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_S))
		pos += vec3(0.0f, 0.0f, -5.0f) * delta_time * speed;
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_A))
		pos += vec3(-5.0f, 0.0f, 0.0f) * delta_time * speed;
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_D))
		pos += vec3(5.0f, 0.0f, 0.0f) * delta_time * speed;

	cam.move(pos);

	cam.update(delta_time);

	cursor_x = current_x;
	cursor_y = current_y;

	return;
};

bool update(double delta_time) {
	static double t = 0.0;
	static double accumulator = 0.0;
	accumulator += delta_time;

	while (accumulator > physics_tick) {
		UpdatePhysics(t, physics_tick);
		accumulator -= physics_tick;
		t += physics_tick;
	}

	for (auto &e : SceneList) {
		e->Update(delta_time);
	}
	phys::Update(delta_time);
	FreeCam(delta_time);
	return true;
}

bool render() {
	for (auto &e : SceneList) {
		e->Render();
	}
	phys::DrawScene();
	return true;

	// Render meshes
	for (auto &e : SceneList)
	{
		auto b = e->GetComponents("Graphics?");
		if (b.size() == 1)//Check only 1 phys component
		{
			const auto p = static_cast<cPhysics *>(b[0]);
		}
		// Bind effect
		renderer::bind(eff);
		// Create MVP matrix
		//auto M = e.get_transform().get_transform_matrix();
		auto M = e->GetTranform();
		auto V = cam.get_view();
		auto P = cam.get_projection();
		auto MVP = P * V * M;
		// Set MVP matrix uniform
		glUniformMatrix4fv(
			eff.get_uniform_location("MVP"), // Location of uniform
			1, // Number of values - 1 mat4
			GL_FALSE, // Transpose the matrix?
			value_ptr(MVP)); // Pointer to matrix data

							 // Bind and set texture
		renderer::bind(tex, 0);
		glUniform1i(eff.get_uniform_location("tex"), 0);

		// Render mesh
		renderer::render(e);
	}

	return true;
}

void main() {
	// Create application
	app application;
	// Set load content, update and render methods
	application.set_load_content(load_content);
	application.set_initialise(initialise);
	application.set_update(update);
	application.set_render(render);
	// Run application
	application.run();
}