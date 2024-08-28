#include "apphandle.hpp"
#include "game/scenes/main_scene.hpp"
AppHandle* AppHandle::s_instance = nullptr;
void AppHandle::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    m_window = glfwCreateWindow(1280, 720, "Hello World", NULL, NULL);
    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, &m_world);
    glfwSetKeyCallback(m_window, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
        GameWorld* world = (GameWorld*)glfwGetWindowUserPointer(win);
        world->keyCallback(static_cast<roj::Keycode>(key), static_cast<roj::InputAction>(action));
        });

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* win, int button, int action, int mods) {
        GameWorld* world = (GameWorld*)glfwGetWindowUserPointer(win);
        world->mouseCallback(static_cast<roj::MouseButton>(button), static_cast<roj::InputAction>(action));
        });

    glfwSetCursorPosCallback(m_window, [](GLFWwindow* win, double x, double y){
        GameWorld* world = (GameWorld*)glfwGetWindowUserPointer(win);
        world->cursorCallback(x, y);
    });

    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1); // Enable vsync
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

AppHandle::AppHandle()
{
    assert(!s_instance && "Application already exists!");
    AppHandle::s_instance = this;

    initWindow();
    uint32_t mainScene = m_world.addScene<MainScene>(roj::SceneFlags::PHYSX_FLAG);
    m_world.setActiveScene(mainScene);
    lastFrame = static_cast<float>(glfwGetTime());
}

void AppHandle::update()
{
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, true);

    m_world.update(deltaTime);
}

void AppHandle::render()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_world.render();
}
void AppHandle::loop()
{

    while (!glfwWindowShouldClose(m_window))
    {
        update();
        render();
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
    glfwTerminate();
}
