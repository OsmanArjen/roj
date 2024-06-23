#include "apphandle.hpp"

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
}
AppHandle::AppHandle()
{
    initWindow();
    uint32_t mainScene = m_world.addScene({
        .init   = mainscene::init,
        .render = mainscene::render,
        .update = mainscene::update,
        .keyCallback    = mainscene::keyCallback,
        .mouseCallback  = mainscene::mouseCallback,
        .cursorCallback = mainscene::cursorCallback});

    m_world.setActiveScene(mainScene);
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
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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
