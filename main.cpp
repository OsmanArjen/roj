#include <cstdint>
#include <cstring>

#include <SDL3/SDL_scancode.h>

#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <app/log.h>
#include <app/clock.h>
#include <app/input.h>
#include <app/platform.h>
#include <gfx/renderer.h>


#include <asset/import.h>
#include <asset/scene.h>
#include <asset/resource.h>
#include <asset/texture.h>

#include <core/system.h>
#include <entity/camera.h>
#include <entity/tps_camera.h>
#include <entity/transform.h>
#include <entity/animation.h>
#include <editor/scene.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_bgfx.h>
#include <entt/tools/davey.hpp>

enum AppAction : roj::ActionMask {
    ActNudgeUp = roj::ActUser << 0,   // R
    ActNudgeDown = roj::ActUser << 1,   // F
    ActTogglePause = roj::ActUser << 2,   // P
};

static const roj::KeyBinding kAppBindings[] = {
    { SDL_SCANCODE_R, ActNudgeUp },
    { SDL_SCANCODE_F, ActNudgeDown },
    { SDL_SCANCODE_P, ActTogglePause },
};



std::string mapPath(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string map_name = "../../../assets/scene/level_blockout.glb"; // Default empty

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--map" && i + 1 < args.size()) {
            map_name = args[i + 1];
            break; // Found it, we can stop looking
        }
    }
    return map_name;
}

void onModelImport(entt::registry& reg, entt::entity asset) {
    auto& asset_v = reg.get<roj::ModelAsset>(asset);
    reg.emplace<roj::ModelScene>(asset, roj::loadModel(reg, asset_v));
}

void initResources(entt::registry& reg) {
    reg.ctx().emplace<tf::Executor>(std::thread::hardware_concurrency());
    roj::resourceType<std::shared_ptr<bimg::ImageContainer>, std::filesystem::path>(reg).connect<&roj::loadTextureFile>();
    roj::resourceType<std::shared_ptr<bimg::ImageContainer>, roj::EmbeddedTexture>(reg).connect<&roj::readEmbeddedTexture>();
    roj::resourceType<roj::ModelAsset, std::filesystem::path>(reg).connect<&roj::importModel>();
    reg.on_construct<roj::ModelAsset>().connect<&onModelImport>();
}
// A target entity that is waiting for its model to finish loading.
struct ModelRequest { entt::entity asset; };

// Returns immediately. The model is instantiated into the returned entity once its ModelScene exists.
entt::entity requestModel(entt::registry& reg, entt::entity asset) {
    const entt::entity target = reg.create();
    reg.emplace<ModelRequest>(target, asset);
    return target;
}

// Call once per frame, after resourcesUpdate.
void placeLoadedModels(entt::registry& reg) {
    // Collect first, then mutate, the same way resource_detail::commit does.
    std::vector<entt::entity> ready;
    for (auto [target, req] : reg.view<ModelRequest>().each())
        if (reg.all_of<roj::ModelScene>(req.asset)) ready.push_back(target);

    for (const entt::entity target : ready) {
        const auto& scene = reg.get<roj::ModelScene>(reg.get<ModelRequest>(target).asset);
        std::cout << scene.log;
        roj::instantiateModel(entt::handle{ reg, target }, scene);
        reg.remove<ModelRequest>(target);
    }
}
int main(int argc, char** argv) {
    const std::string mapPathStr = mapPath(argc, argv);

	roj::PlatformInit pinit;
	roj::ClockInit cinit;
	roj::GfxInit rinit;

    pinit.title = "Roj Engine";
    pinit.width = 1980;
    pinit.height = 1080;
    pinit.extraBindings = kAppBindings;
    pinit.extraBindingCount = uint32_t(sizeof(kAppBindings) / sizeof(kAppBindings[0]));
    cinit.fixedHz = 60.0f;

    roj::GfxContext renderer;
    roj::Platform platform;
    roj::Clock clock;
    roj::Input input;

    entt::registry registry;

    if (!roj::platformInit(platform, pinit))
        return 1;

    rinit.nwh = platform.nativeWindow;
    rinit.resolution = platform.pixelSize;
    if (!roj::gfxInitialize(registry, renderer, rinit)) {
        roj::platformShutdown(platform);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOther(platform.window);
    ImGui_Implbgfx_Init(255, 0);   // 255 = main ImGui view ID (drawn on top); floating windows use the ids right below it

    roj::clockInit(clock, cinit);

    roj::Camera camera;
    roj::ThirdPersonController tps;
    glm::vec3 knightPos{ 0.0f };
    float knightYaw = 0.0f;                       // radians about +Y
    const float knightSpeed = 4.0f;
    const float modelYawOffset = 0.0f;            // glm::pi<float>() if the knight faces -Z

    const char* romero = "../../../assets/scene/Solus_the_knight.gltf";

    initResources(registry);
    auto mapAsset_v = roj::resourceLoad<roj::ModelAsset>(registry, std::filesystem::path{ mapPathStr });
    auto romeroAsset_v = roj::resourceLoad<roj::ModelAsset>(registry, std::filesystem::path{ romero });

    requestModel(registry, mapAsset_v);
    const entt::entity fpsModel = requestModel(registry, romeroAsset_v);

    const auto fpsDefaultRotation = glm::quat(glm::radians(glm::vec3(1.0f, 187.0f, 1.0f)));
    const auto fpsOffset = glm::vec3(0.100f, 0.0f, 0.120f);
   
    roj::SceneEditor editor;
    while (!input.quit) {
        roj::clockTick(clock);
        roj::platformPollEvents(platform, input, &ImGui_ImplSDL3_ProcessEvent);

        roj::resourcesUpdate(registry, std::chrono::steady_clock::now() + std::chrono::milliseconds(1));
        placeLoadedModels(registry);      // freshly loaded scenes -> entities

        const glm::vec3 dir = roj::thirdPersonMoveDirection(&camera, input);
        knightPos += dir * knightSpeed * clock.dtReal;
        knightYaw = roj::thirdPersonFacingYaw(knightYaw, dir, glm::radians(720.0f), clock.dtReal, modelYawOffset);

        // Only touch the knight's entity once it has actually been instantiated.
        if (!registry.all_of<ModelRequest>(fpsModel)) {
            registry.patch<roj::Transform>(fpsModel, [&](roj::Transform& t) {
                t.position = knightPos;
                t.rotation = glm::vec3(0.0f, knightYaw, 0.0f);
                });

            const auto& clips = registry.get<roj::ModelScene>(romeroAsset_v).animations;
            const bool moving = input.held & (roj::ActMoveForward | roj::ActMoveBack | roj::ActMoveLeft | roj::ActMoveRight);
            roj::animationEnsure(registry, fpsModel, moving ? clips[5] : clips[15]);   // before animationUpdate
        }

        roj::thirdPersonUpdate(&tps, &camera, knightPos, input, clock.dtReal);   // after the knight moved

        if (roj::inputPressed(&input, ActTogglePause))
            roj::platformSetMouseCaptured(platform, !platform.mouseCaptured);


		
        roj::animationUpdate(registry, clock.dt);   // writes local poses
        roj::Transform::propagate(registry);        // locals -> world matrices (you already call this)
        roj::skinUpdate(registry);                  // reads world matrices -> palettes

        ImGui_ImplSDL3_NewFrame();
		ImGui_Implbgfx_NewFrame();
        ImGui::NewFrame();

        roj::draw_scene_editor(editor, registry);
        roj::draw_camera_fields(camera);
        //roj::draw_fly_controller_fields(fly);

        ImGui::Render();
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
        ImGui::RenderPlatformWindowsDefault();

        roj::GfxView& view = renderer.forwardView;
        view.origin = glm::ivec2(0);
        view.size = platform.pixelSize;
        const float aspect = view.size.y > 0 ? float(view.size.x) / float(view.size.y) : 1.0f;

        roj::cameraViewMatrix(&camera, glm::value_ptr(view.view));
        roj::cameraProjMatrix(&camera, aspect, bgfx::getCaps()->homogeneousDepth, glm::value_ptr(view.projection));
        view.eye =  camera.position;

        roj::gfxSubmit(renderer, registry);
    }

    ImGui_Implbgfx_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    roj::gfxShutdown(renderer, registry);
    roj::platformShutdown(platform);
    return 0;
} 