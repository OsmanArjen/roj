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
				staticModelLoader.load(modelPath.string());
				m_resources.models[modelPath.filename().string()] = std::move(staticModelLoader.get());

			}
		}
	}

	////////////////////////FOR THE FUCKING TEST MAP IDC BRO
	roj::MeshTexture a = m_resources.models["test.obj"].back().textures.back();
	for (roj::Mesh& mesh : m_resources.models["test.obj"])
	{
		if (mesh.textures.size() == 0) mesh.textures.push_back(a);
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
	m_resources.shaderObjects["g_buffer"].link("res/shaders/g_buffer.vs", "res/shaders/g_buffer.fs");
	m_resources.shaderObjects["skybox"].link("res/shaders/skybox.vs", "res/shaders/skybox.fs");

	m_resources.shaderObjects["lighting_deffered"].link("res/shaders/lighting_deffered.vs", "res/shaders/lighting_deffered.fs");
	m_resources.shaderObjects["lighting_apply"].link("res/shaders/lighting_apply.vs", "res/shaders/lighting_apply.fs");

	m_resources.shaderObjects["shadow_depth"].link("res/shaders/shadow_mapping _depth.vs", "res/shaders/shadow_mapping _depth.fs", "res/shaders/shadow_mapping _depth.gs");
	m_resources.shaderObjects["shadow_apply"].link("res/shaders/shadow_mapping_apply.vs", "res/shaders/shadow_mapping_apply.fs");

	std::cout << m_resources.shaderObjects["shadow_depth"].getInfoLog() << '\n';
	std::cout << m_resources.shaderObjects["lighting_apply"].getInfoLog() << '\n';
	auto& lightPass = m_resources.shaderObjects["lighting_deffered"];
	lightPass.use();
	lightPass.uniform1i("gPosition", 0);
	lightPass.uniform1i("gNormal", 1);
	lightPass.uniform1i("shadowMap", 2);

	auto& renderPass = m_resources.shaderObjects["lighting_apply"];
	renderPass.use();
	renderPass.uniform1i("gAlbedoSpec", 0);
	renderPass.uniform1i("gEmissive", 1);
	renderPass.uniform1i("gLightmap", 2);

}

void MainScene::initSceneLights()
{
	SceneLight light;
	light.position = {0, 0,0};
	light.color = glm::vec3(1);
	light.radius = 6000.f;
	light.shadowMapIdx = 0;
	m_lights.emplace_back(std::move(light));
	
}

void MainScene::initGBuffer()
{
	glm::uvec2 winSize = utils::getWindowSize();
	glGenFramebuffers(1, &m_gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);
	glDisable(GL_BLEND);
	glGenTextures(1, &m_gPosition);
	glBindTexture(GL_TEXTURE_2D, m_gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winSize.x, winSize.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gPosition, 0);

	glGenTextures(1, &m_gNormal);
	glBindTexture(GL_TEXTURE_2D, m_gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, winSize.x, winSize.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_gNormal, 0);

	glGenTextures(1, &m_gAlbedoSpec);
	glBindTexture(GL_TEXTURE_2D, m_gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, winSize.x, winSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_gAlbedoSpec, 0);

	glGenTextures(1, &m_gEmissive);
	glBindTexture(GL_TEXTURE_2D, m_gEmissive);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, winSize.x, winSize.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_gEmissive, 0);

	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);


	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, winSize.x, winSize.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	/////////////////////////////////////////////////////////
	glGenFramebuffers(1, &m_lightBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_lightBuffer);
	glGenTextures(1, &m_gLightmap);
	glBindTexture(GL_TEXTURE_2D, m_gLightmap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winSize.x, winSize.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gLightmap, 0);

	unsigned int attachments2[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments2);
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

	const uint32_t SHADOW_WIDTH = 1024;
	const uint32_t SHADOW_HEIGHT = 1024;
	glGenFramebuffers(1, &m_shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
	glGenTextures(1, &m_shadowCubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_shadowCubeMap);
	for (unsigned int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
			SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_shadowCubeMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	const int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
		throw 0;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
	controller->move(physx::PxVec3{ 0.0f, 800.0f,  0.0f }, 0.01f, 1.0f, physx::PxControllerFilters());
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
	if (roj::getKeyPressed(roj::P))
	{
		m_lights.back().position = m_entities.get<game::Transform>(player).position;
	}
	movement = glm::normalize(movement) * speed;

	controller->move(physx::PxVec3{ movement.x, 0.0f, movement.z }, 0.01f, deltatime, physx::PxControllerFilters());
	
}

void MainScene::renderNonScene()
{

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

				if (ImGui::BeginTable("##entityTable", 1, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter, { 0, 100 }))
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
		if (m_entities.any_of<game::Transform>(m_editorHandle.selectedEntity))
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
	, m_camera{ {0.0f, 1.0f, 0.0f}, 0,0 }
{
	loadModels();
	loadShaders();
	initSceneLights();
	initGBuffer();
	initSceneMap();
	initScenePhysics();
	initPlayer();
}

void MainScene::renderGbuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	auto& skyshader = m_resources.shaderObjects["skybox"];
	skyshader.use();
	skyshader.uniform1i("skybox", 0);
	skyshader.uniformMat4("view", m_camera.getViewMatrix({}));
	skyshader.uniformMat4("projection", glm::perspective(glm::radians(45.f), (float)1280 / (float)720, 0.1f, 10000.0f));
	m_skybox.render();
	glm::uvec2 winSize = utils::getWindowSize();
	auto viewPos = m_entities.get<game::Transform>(player).position;
	auto viewMatrix = m_camera.getViewMatrix(viewPos);
	auto renderables = m_entities.view<game::Transform, game::Renderable>();
	auto& gshader = m_resources.shaderObjects["g_buffer"];
	gshader.use();
	gshader.uniformMat4("view", viewMatrix);
	gshader.uniform3f("viewPos", viewPos);
	gshader.uniformMat4("projection", glm::perspective(glm::radians(45.f), (float)1280 / (float)720, 0.1f, 10000.0f));
	for (auto [entity, transform, renderable] : renderables.each())
	{
		gshader.uniformMat4("model", glm::scale(glm::translate(glm::mat4(1.0), transform.position), glm::vec3(10.0)));
		game::renderModel(m_resources.models[renderable.model], gshader);
	}
}

void MainScene::renderShadowBuffer()
{
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	const uint32_t SHADOW_WIDTH = 1024;
	const uint32_t SHADOW_HEIGHT = 1024;
	auto& lightPos = m_lights.back().position;
	float near_plane = 1.0f;
	float far_plane = 25000;
	glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
	std::vector<glm::mat4> shadowTransforms;
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

	auto& depthShader = m_resources.shaderObjects["shadow_depth"];
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	depthShader.use();
	for (unsigned int i = 0; i < 6; ++i)
		depthShader.uniformMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
	depthShader.uniform1f("far_plane", far_plane);
	depthShader.uniform3f("lightPos", lightPos);
	auto renderables = m_entities.view<game::Transform, game::Renderable>();
	for (auto [entity, transform, renderable] : renderables.each())
	{
		depthShader.uniformMat4("model", glm::scale(glm::translate(glm::mat4(1.0), transform.position), glm::vec3(10.0)));
		game::renderModel(m_resources.models[renderable.model], depthShader);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glm::uvec2 winSize = utils::getWindowSize();
	glViewport(0, 0, winSize.x, winSize.y);
}

void MainScene::renderLightBuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_lightBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	auto viewPos = m_entities.get<game::Transform>(player).position;
	auto& lightPass = m_resources.shaderObjects["lighting_deffered"];
	lightPass.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_shadowCubeMap);

	glBindVertexArray(m_quadVAO);
	for (SceneLight& light : m_lights)
	{
		lightPass.uniform3f("light.Position", light.position);
		lightPass.uniform3f("light.Color", light.color);
		lightPass.uniform1f("light.radius", light.radius);
		lightPass.uniform3f("viewPos", viewPos);
		lightPass.uniform1f("far_plane", 25000.0f);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}

void MainScene::applyLighting()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	auto& renderPass = m_resources.shaderObjects["lighting_apply"];
	renderPass.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_gAlbedoSpec);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_gEmissive);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_gLightmap);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
	glm::uvec2 winSize = utils::getWindowSize();
	glBlitFramebuffer(0, 0, winSize.x, winSize.y, 0, 0, winSize.x, winSize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MainScene::renderScene()
{
	renderShadowBuffer();

	renderGbuffer();
	renderLightBuffer();
	applyLighting();
}

MainScene::~MainScene() {}

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



