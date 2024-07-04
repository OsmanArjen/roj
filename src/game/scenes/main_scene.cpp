#include "game/scenes/main_scene.hpp"

namespace mainscene
{

	void init(roj::Scene& scene, GameResource& resources)
	{
		roj::ModelLoader<roj::SkinnedMesh> modelLoader;
		
		modelLoader.load("res/assets/models/viewmodel/untitled.glb");
		resources.skinnedModels.emplace_back(std::move(modelLoader.get()));
		resources.shaderObjects["basic"].link("res/shaders/vs_basic.glsl", "res/shaders/fs_basic.glsl");
		
		auto entity = scene.entities.create();

		scene.entities.emplace<game::Transform>(entity);
		scene.entities.emplace<game::Renderable>(entity, (uint32_t)resources.skinnedModels.size()-1, "basic");
		scene.entities.emplace<roj::Animator>(entity, resources.skinnedModels[0]).set("Action.001");

		scene.camera = { { -235.0f, 244.0f, 181.0f }, {0.0f, 1.0f, 0.0f}, -34.1f, -27.1f };
	}

	void update(roj::Scene& scene)
	{
		auto enttView = scene.entities.view<roj::Animator>();
		for (auto [entity, animator] : enttView.each())
		{
			animator.update(scene.deltatime);
		}
	}

	void render(roj::Scene& scene, GameResource& resources)
	{
		auto viewPos = scene.camera.getPosition();
		auto viewMatrix = scene.camera.getViewMatrix();
		auto enttView = scene.entities.view<game::Transform, game::Renderable, roj::Animator>();
		for (auto [entity, transform, renderable, animator] : enttView.each())
		{

			roj::GLShaderObject& shader = resources.shaderObjects[renderable.shader];
			shader.use();

			auto modelMatrix = glm::translate(glm::mat4(1.0f), transform.position);

			shader.uniform3f("viewPos", viewPos);
			shader.uniformMat4("view", viewMatrix);
			shader.uniformMat4("model", glm::scale(modelMatrix, glm::vec3(transform.scale)));
			shader.uniformMat4("projection", glm::perspective(glm::radians(45.f), (float)1280 / (float)720, 0.1f, 1000.0f));
			auto transforms = animator.getBoneMatrices();
			for (int i = 0; i < transforms.size(); ++i)
				shader.uniformMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
			game::renderModel(resources.skinnedModels[renderable.modelIdx], shader);
		}
	}

	void keyCallback(roj::Scene& scene, roj::Keycode key, roj::InputAction action)
	{
		
		if ((key == roj::Keycode::W) && (action & roj::InputAction::Press))
			scene.camera.move(roj::Camera::FORWARD, 200.5f, scene.deltatime);
		if ((key == roj::Keycode::S) && (action & roj::InputAction::Press))
			scene.camera.move(roj::Camera::BACKWARD, 200.5f, scene.deltatime);
		if ((key == roj::Keycode::A) && (action & roj::InputAction::Press))
			scene.camera.move(roj::Camera::LEFT, 200.5f, scene.deltatime);
		if ((key == roj::Keycode::D) && (action & roj::InputAction::Press))
			scene.camera.move(roj::Camera::RIGHT, 200.5f, scene.deltatime);
		if ((key == roj::Keycode::F) && (action & roj::InputAction::Press))
		{
			auto enttView = scene.entities.view<roj::Animator>();
			for (auto [entity, animator] : enttView.each())
			{
				animator.reset();
			}
		}
	}

	void mouseCallback(roj::Scene& scene, roj::MouseButton button, roj::InputAction action)
	{

	}

	void cursorCallback(roj::Scene& scene, float xpos_p, float ypos_p)
	{
		static float lastX = xpos_p;
		static float lastY = ypos_p;

		float xoffset = xpos_p - lastX;
		float yoffset = lastY - ypos_p; // reversed since y-coordinates go from bottom to top
		scene.camera.rotate(xoffset, yoffset, 0.1f);

		lastX = xpos_p;
		lastY = ypos_p;
	}

}

