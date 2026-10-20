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


#include <asset/model.h>
#include <core/system.h>
#include <entity/camera.h>
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

int main(int argc, char** argv) {

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

    const char* modelPath = "../../../assets/scene/starwars.glb";
    roj::ModelScene mapScene = roj::loadModel(registry, modelPath);
    entt::entity mapRoot = registry.create();
    roj::instantiateModel(entt::handle{ registry, mapRoot }, mapScene);
    //roj::animationPlay(registry, mapRoot, mapScene.animations[0]);


    roj::Camera camera;
    roj::FlyController fly;
    
    roj::SceneEditor editor;
    while (!input.quit) {
        roj::clockTick(clock);
		roj::platformPollEvents(platform, input, &ImGui_ImplSDL3_ProcessEvent);
        roj::flyControllerUpdate(&fly, &camera, input, clock.dtReal);

		if (roj::inputPressed(&input, ActTogglePause)) {
			roj::platformSetMouseCaptured(platform, !platform.mouseCaptured);
		}

		
        roj::animationUpdate(registry, clock.dt);   // writes local poses
        roj::Transform::propagate(registry);        // locals -> world matrices (you already call this)
        roj::skinUpdate(registry);                  // reads world matrices -> palettes

        ImGui_ImplSDL3_NewFrame();
		ImGui_Implbgfx_NewFrame();
        ImGui::NewFrame();

        roj::draw_scene_editor(editor, registry);
        roj::draw_camera_fields(camera);
        roj::draw_fly_controller_fields(fly);

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