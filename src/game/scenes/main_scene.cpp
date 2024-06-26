#include "game/scenes/main_scene.hpp"

namespace mainscene
{

	void init(roj::Scene& scene, GameResource& resources)
	{
		roj::ModelLoader<roj::SkinnedMesh> modelLoader;
		modelLoader.load("res/assets/models/viewmodel/scene.gltf");
		resources.skinnedModels.emplace_back(std::move(modelLoader.get()));
		roj::Animator animator(resources.skinnedModels[0]);

		for (auto& name : animator.get()) { std::cout << name << '\n'; }
		animator.set("allanims");
		resources.shaderObjects["basic"].link("res/shaders/vs_basic.glsl", "res/shaders/fs_basic.glsl");
		scene.camera = { glm::vec3(0.0) };
		scene.entities.emplace<roj::Animator>(scene.entities.create(), animator);
		//TODO init an entity and render
	}

	void update(roj::Scene& scene)
	{
		auto view = scene.entities.view<roj::Animator>();
		std::get<0>(view.get(entt::entity(0))).update(scene.deltatime);
	}

	void render(roj::Scene& scene, GameResource& resources)
	{
		auto viewPos = scene.camera.getPosition();
		auto viewMatrix = scene.camera.getViewMatrix();
		auto enttView = scene.entities.view<game::Transform, game::Renderable>();
		for (auto [entity, transform, renderable] : enttView.each())
		{
			roj::GLShaderObject& shader = resources.shaderObjects[renderable.shader];
			auto modelMatrix = glm::translate(glm::mat4(1.0f), transform.position);

			shader.uniform3f("viewPos", viewPos);
			shader.uniformMat4("view", viewMatrix);
			shader.uniformMat4("model", glm::scale(modelMatrix, glm::vec3(transform.scale)));

			game::renderModel(resources.models[renderable.modelIdx], shader);
		}
	}

	void keyCallback(roj::Scene& scene, roj::Keycode key, roj::InputAction action)
	{
		if ((key == roj::Keycode::W) && (action & roj::InputAction::Press))
			scene.camera.move(roj::Camera::FORWARD, 2.5f, scene.deltatime);
		else if ((key == roj::Keycode::S) && (action & roj::InputAction::Press))
			scene.camera.move(roj::Camera::BACKWARD, 2.5f, scene.deltatime);
		else if ((key == roj::Keycode::A) && (action & roj::InputAction::Press))
			scene.camera.move(roj::Camera::LEFT, 2.5f, scene.deltatime);
		else if ((key == roj::Keycode::D) && (action & roj::InputAction::Press))
			scene.camera.move(roj::Camera::RIGHT, 2.5f, scene.deltatime);
	}

	void mouseCallback(roj::Scene& scene, roj::MouseButton button, roj::InputAction action)
	{

	}

	void cursorCallback(roj::Scene& scene, float xpos_p, float ypos_p)
	{
		static float lastX = 0.0f;
		static float lastY = 0.0f;
		float xpos = static_cast<float>(xpos_p);
		float ypos = static_cast<float>(ypos_p);

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
		scene.camera.rotate(xoffset, yoffset, 0.1f);

		lastX = xpos;
		lastY = ypos;
	}

}

