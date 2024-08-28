#include "game/scenes/main_scene.hpp"
#include <filesystem>

void MainScene::loadModels()
{
	roj::ModelLoader<roj::SkinnedMesh> modelLoader;
	roj::ModelLoader staticModelLoader;

	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

	for (const auto& dirEntry : recursive_directory_iterator("res/assets/models"))
	{
		if (!dirEntry.is_directory())
		{
			auto& modelPath = dirEntry.path();
			auto ext = modelPath.extension();
			if (ext == ".obj")
			{
				std::cout << modelPath;
				staticModelLoader.load(modelPath.string());
				m_resources.models[modelPath.filename().string()] = std::move(staticModelLoader.get());
			}
			else
			{
				modelLoader.load(modelPath.string());
				m_resources.skinnedModels[modelPath.filename().string()] = std::move(modelLoader.get());
			}
		}
	}

	std::vector<std::string> faces
	{
		"res/assets/textures/skybox/PositiveX.png",
		"res/assets/textures/skybox/NegativeX.png",
		"res/assets/textures/skybox/PositiveY.png",
		"res/assets/textures/skybox/NegativeY.png",
		"res/assets/textures/skybox/PositiveZ.png",
		"res/assets/textures/skybox/NegativeZ.png",
	};

	m_skybox = roj::Skybox::create(faces);
}

void MainScene::loadShaders()
{
	m_resources.shaderObjects["animated"].link("res/shaders/skeletal.vs", "res/shaders/basic.fs");
	m_resources.shaderObjects["basic"].link("res/shaders/basic.vs", "res/shaders/basic.fs");
	m_resources.shaderObjects["skybox"].link("res/shaders/skybox.vs", "res/shaders/skybox.fs");
	m_resources.shaderObjects["g_buffer"].link("res/shaders/g_buffer.vs", "res/shaders/g_buffer.fs");
	m_resources.shaderObjects["lighting_deffered"].link("res/shaders/lighting_deffered.vs", "res/shaders/lighting_deffered.fs");
}
void MainScene::initGBuffer()
{
	glm::uvec2 winSize = utils::getWindowSize();
	glGenFramebuffers(1, &m_gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);

	glGenTextures(1, &m_gPosition);
	glBindTexture(GL_TEXTURE_2D, m_gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winSize.x, winSize.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gPosition, 0);

	glGenTextures(1, &m_gNormal);
	glBindTexture(GL_TEXTURE_2D, m_gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winSize.x, winSize.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_gNormal, 0);

	glGenTextures(1, &m_gAlbedoSpec);
	glBindTexture(GL_TEXTURE_2D, m_gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, winSize.x, winSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_gAlbedoSpec, 0);

	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, winSize.x, winSize.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	float quadVertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};

	uint32_t quadVBO;
	glGenVertexArrays(1, &m_quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(m_quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}
void MainScene::initSceneMap()
{
	m_sceneMap = m_entities.create();
	m_entities.emplace<game::Transform>(m_sceneMap);
	m_entities.emplace<game::Renderable>(m_sceneMap, "test.obj", "g_buffer");
}

void MainScene::initScenePhysics()
{
	physx::PxSceneDesc sceneDesc(m_physxHandle.physics->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = m_physxHandle.dispatcher;
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
	m_physxScene = m_physxHandle.physics->createScene(sceneDesc);

	auto pxMaterial = m_physxHandle.physics->createMaterial(0.5f, 0.5f, 0.5f);
	physx::PxRigidStatic* groundPlane = PxCreatePlane(*m_physxHandle.physics, physx::PxPlane(0, 1, 0, 50), *pxMaterial);
	m_physxScene->addActor(*groundPlane);

}

void MainScene::initPlayer()
{
	player = m_entities.create();
	m_entities.emplace<game::Transform>(player);

	auto pxMaterial = m_physxHandle.physics->createMaterial(1.0f, 1.0f, 1.0f);
	physx::PxCapsuleControllerDesc desc;
	desc.height = 400.f;
	desc.radius = 50.f;
	desc.position = physx::PxExtendedVec3(0, 0, 0);
	desc.material = pxMaterial;
	desc.slopeLimit = 0.0f;
	desc.contactOffset = 0.1f;
	desc.stepOffset = 0.5f;
	desc.invisibleWallHeight = 0.0f;
	desc.maxJumpHeight = 0.0f;
	desc.reportCallback = nullptr;
	desc.behaviorCallback = nullptr;
	desc.userData = nullptr;

	auto manager = PxCreateControllerManager(*m_physxScene);
	auto controller = manager->createController(desc);
	m_entities.emplace<physx::PxController*>(player, controller);
	m_entities.emplace<physx::PxRigidDynamic*>(player, controller->getActor());
	controller->move(physx::PxVec3{ 0.0f, 400.0f,  0.0f }, 0.01f, 1.0f, physx::PxControllerFilters());
}

void MainScene::updateInput(float deltatime)
{
	auto controller = m_entities.get<physx::PxController*>(player);
	float speed = 20.0f;
	glm::vec3 front = m_camera.getFront();
	front = { front.x, 0, front.z };
	glm::vec3 right = m_camera.getRight();

	glm::vec3 movement = glm::vec3(0.0f);

	if (roj::getKeyPressed(roj::W)) {
		movement += front;
	}
	if (roj::getKeyPressed(roj::S)) {
		movement -= front;
	}
	if (roj::getKeyPressed(roj::D)) {
		movement += right;
	}
	if (roj::getKeyPressed(roj::A)) {
		movement -= right;
	}

	movement = glm::normalize(movement) * speed;

	controller->move(physx::PxVec3{ movement.x, 0.0f, movement.z }, 0.01f, deltatime, physx::PxControllerFilters());
}

void MainScene::renderNonScene()
{
	auto& skyshader = m_resources.shaderObjects["skybox"];
	skyshader.use();
	skyshader.uniform1i("skybox", 0);
	skyshader.uniformMat4("view", glm::mat3(m_camera.getViewMatrix({})));
	skyshader.uniformMat4("projection", glm::perspective(glm::radians(45.f), (float)1280 / (float)720, 0.1f, 10000.0f));
	m_skybox.render();
}

void MainScene::renderImgui()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
	ImGui::Begin("Scene", (bool*)0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
	
	if (ImGui::BeginTable("##split", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Entity");
		ImGui::TableSetupColumn(("Components{Entity: " + ((m_editorHandle.selectedEntity == entt::null) ? std::string("None") : std::to_string((uint64_t)m_editorHandle.selectedEntity)) + +"}").c_str());
		ImGui::TableHeadersRow();
		ImGui::TableNextColumn();

		if (ImGui::Button("New", { ImGui::GetContentRegionMax().x, 0 }))
		{
			ImGui::OpenPopup("+Entity Group");
		}

		if (ImGui::BeginPopupModal("+Entity Group", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static char inputText[255];
			ImGui::InputText("##inputText", inputText, IM_ARRAYSIZE(inputText));

			if (ImGui::Button("Add")) {
				// Do something with the input text
				m_editorHandle.enttTypes.emplace(inputText, m_editorHandle.nextGroupTag++);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
		for (auto& [name, typeTag] : m_editorHandle.enttTypes)
		{
			ImGui::BeginChild("EntityChild", ImVec2(0, 0), false);
			if (ImGui::CollapsingHeader(name.c_str()))
			{
				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
				{
					ImGui::OpenPopup("##ContextMenu");
				}

				if (ImGui::BeginPopup("##ContextMenu"))
				{
					if (ImGui::MenuItem(("New:{" + name + "}").c_str()))
					{
						entt::entity newEntity = m_entities.create();
						m_entities.emplace<EntityTags>(newEntity, typeTag);
						m_entities.emplace<game::Transform>(newEntity);
						m_editorHandle.selectedEntity = newEntity;
					}
					ImGui::EndPopup();
				}

				if (ImGui::BeginTable("##entityTable", 1, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter, {0, 100}))
				{
					auto tagView = m_entities.view<EntityTags>();
					for (auto [ent, currTag] : tagView.each())
					{
						if (typeTag == currTag.groupTagId)
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							if (ImGui::Selectable(("ID: " + std::to_string((std::uint64_t)ent)).c_str(), m_editorHandle.selectedEntity == ent))
							{
								m_editorHandle.selectedEntity = ent;
							}

						}

					}
					ImGui::EndTable();
				}


			}
			ImGui::EndChild();


		}
		ImGui::TableNextColumn();
		ImGui::BeginChild("ComponentsChild", ImVec2(0, 0), false);
		if(m_entities.any_of<game::Transform>(m_editorHandle.selectedEntity))
		{
			if (ImGui::CollapsingHeader("Transform"))
			{
				game::Transform& transform = m_entities.get<game::Transform>(m_editorHandle.selectedEntity);
				ImGui::PushID("position");

				ImGui::Text("position:");
				ImGui::DragFloat3("", &transform.position.x, 0.01f);
				ImGui::PopID();

				// Rotation
				ImGui::PushID("rotation");
				ImGui::Text("rotation:");
				ImGui::DragFloat3("", &transform.rotation.x, 0.01f);
				ImGui::PopID();
				// Scale
				ImGui::PushID("scale");

				ImGui::Text("scale:");
				ImGui::DragFloat3("", &transform.scale.x, 0.01f, 0.01);
				ImGui::PopID();

			}
		}

		ImGui::EndChild();
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

MainScene::MainScene(ResourceHandle& resources, PhysxHandle& physxHandle)
	: m_resources(resources), m_physxHandle(physxHandle)
	, m_camera{{0.0f, 1.0f, 0.0f}, 0,0}
{
	loadModels();
	loadShaders();
	initGBuffer();
	initSceneMap();
	initScenePhysics();
	initPlayer();
}

void MainScene::renderScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::uvec2 winSize = utils::getWindowSize();
	auto viewPos = m_entities.get<game::Transform>(player).position;
	auto viewMatrix = m_camera.getViewMatrix(viewPos);
	auto renderables = m_entities.view<game::Transform, game::Renderable>();
	for (auto [entity, transform, renderable] : renderables.each())
	{
		auto& shader = m_resources.shaderObjects[renderable.shader];
		shader.use();
		shader.uniformMat4("view", viewMatrix);
		shader.uniformMat4("model", glm::scale(glm::translate(glm::mat4(1.0), transform.position), glm::vec3(10.0)));
		shader.uniformMat4("projection", glm::perspective(glm::radians(45.f), (float)1280 / (float)720, 0.1f, 10000.0f));

		game::renderModel(m_resources.models[renderable.model], shader);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	auto& lightPass = m_resources.shaderObjects["lighting_deffered"];
	lightPass.use();
	lightPass.uniform1i("gPosition", 0);
	lightPass.uniform1i("gNormal", 1);
	lightPass.uniform1i("gAlbedoSpec", 2);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_gAlbedoSpec);

	lightPass.uniform3f("light.Position", viewPos);
	lightPass.uniform3f("light.Color", glm::vec3(0.5));

	const float linear = 0.7f;
	const float quadratic = 1.8f;
	lightPass.uniform1f("light.Linear", linear);
	lightPass.uniform1f("light.Quadratic", quadratic);

	lightPass.uniform3f("viewPos", viewPos);
	glBindVertexArray(m_quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer

	glBlitFramebuffer(0, 0, winSize.x, winSize.y, 0, 0, winSize.x, winSize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

MainScene::~MainScene(){}

void MainScene::update(float deltatime)
{
	updateInput(deltatime);

	auto enttPhysxView = m_entities.view<game::Transform, physx::PxRigidDynamic*>();
	for (auto [entity, transform, body] : enttPhysxView.each())
	{
		auto pose = body->getGlobalPose();
		transform.position = { pose.p.x, pose.p.y, pose.p.z };
		transform.rotation = transform.rotation.wxyz(pose.q.w, pose.q.x, pose.q.y, pose.q.z);
	}

	m_physxScene->simulate(1.0f / 10.0f);
	m_physxScene->fetchResults(true);
}

void MainScene::render()
{
	renderNonScene();
	renderScene();

	if (m_editorHandle.open)
	{
		renderImgui();
	}
}

void MainScene::keyCallback(roj::Keycode key, roj::InputAction action)
{
	if ((key == roj::Keycode::F) && (action & roj::InputAction::Press))
	{
		m_editorHandle.open = !m_editorHandle.open;
		roj::enableCursor(m_editorHandle.open);
	}
}

void MainScene::mouseCallback(roj::MouseButton button, roj::InputAction action)
{

}

void MainScene::cursorCallback(float xpos_p, float ypos_p)
{
	static float lastX = xpos_p;
	static float lastY = ypos_p;

	float xoffset = xpos_p - lastX;
	float yoffset = lastY - ypos_p; 
	if (not m_editorHandle.open)
	{
		m_camera.rotate(xoffset, yoffset, 0.1f);
	}

	lastX = xpos_p;
	lastY = ypos_p;
}



