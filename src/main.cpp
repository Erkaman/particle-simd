#include "gl_util.hpp"

#include "fps_manager.hpp"

#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "lodepng.h"

using std::string;
using std::vector;
using glm::vec3;

/*
  Global variables below.
*/

GLuint vao;

const int WINDOW_WIDTH = 1248;
const int WINDOW_HEIGHT = 845;
const int GUI_WIDTH = 250;

GLFWwindow* window;

float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
float cameraZoom = 30.8f;
glm::vec3 cameraPos;
glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;

GLuint normalShader;

double prevMouseX = 0;
double prevMouseY = 0;

double curMouseX = 0;
double curMouseY = 0;

struct Constraint{
public:
    GLuint i0;
    GLuint i1;
    float restLength;

    Constraint(GLuint i0_, GLuint i1_, float restLength_) : i0(i0_), i1(i1_), restLength(restLength_){ }

};

class Cloth {
private:
    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_oldPositions;

    std::vector<Constraint> m_constraints;

    GLuint m_vertexVbo;

    const int N = 10; // degree of tesselation. means  quads.

public:

    void AddConstraint(GLuint i0, GLuint i1) {
        m_constraints.emplace_back(i0, i1, glm::distance(m_positions[i0], m_positions[i1]  ) );
    }

    Cloth() {
        const GLuint base = 0;

        int start = 0;


        float size = 5.5f;

        float xmin = -size;
        float xmax = +size;
        float ymin = -size;
        float ymax = +size;

        for (int row = 0; row <= N; ++row) {

            float z = (row / (float)N)*(ymax - ymin) + ymin;
            float v = row / (float)N;

            for (int col = 0; col <= N; ++col) {

                float x = (col / (float)N)*(xmax - xmin) + xmin;
                float u = col / (float)N;

                GLuint i0 = m_positions.size();
                m_positions.push_back(glm::vec3(x, 0.0f, z));
                m_oldPositions.push_back(glm::vec3(x, 0.0f, z));

            }

        }
        int end = m_positions.size();


        for (int row = 0; row <= N; ++row) {

            for (int col = 0; col <= N; ++col) {

                int i = row * (N + 1) + col;

                int i0 = i + 0;
                int i1 = i + 1;
                int i2 = i + (N + 1) + 0;
                int i3 = i + (N + 1) + 1;


                // add constraint linked to the next element in the next column, if it exist.
                if (col < N)
                    AddConstraint(i0, i1);

                // add constraint linked to the next element in the next row, if it exists
                if (row < N)
                    AddConstraint(i0, i2);
            }
        }

        GL_C(glGenBuffers(1, &m_vertexVbo));
    }

    int counter = 0;
    float totalDelta = 0.0;

    void Update(float delta) {

        counter++;
        delta = 0.016;
        totalDelta += delta;

        for (size_t i = 0; i < m_positions.size(); ++i) {
            vec3 vel = m_positions[i] - m_oldPositions[i];

            vec3 g(0.0f, -2.4f, 0.0f);
            vec3 next = m_positions[i];
            next += vel;
            next +=  g * delta * delta;

            m_oldPositions[i] = m_positions[i];
            m_positions[i] = next;
        }

        for (int i = 0; i < 1; ++i){
            for (const Constraint& c : m_constraints) {

                const GLuint i0 = c.i0;
                const GLuint i1 = c.i1;

                vec3& v0 = m_positions[i0];
                vec3& v1 = m_positions[i1];

                vec3 d = v1 - v0;
                float dLength = glm::length(d);

                float diff = (dLength - c.restLength) / dLength;
                v0 += d*0.5f*diff;
                v1 -= d*0.5f*diff;

            }
        }

        // force this guy to stay fixed at one point.
        for (size_t i = 0; i <= N; ++i)
            m_positions[i] = m_oldPositions[i];

        GL_C(glBindBuffer(GL_ARRAY_BUFFER, m_vertexVbo));
        GL_C(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*m_positions.size(), m_positions.data(), GL_DYNAMIC_DRAW));
    }

    void Render() {

        GL_C(glEnableVertexAttribArray(0));
        GL_C(glBindBuffer(GL_ARRAY_BUFFER, m_vertexVbo));
        GL_C(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));

        GL_C(glPointSize(4.0));

        GL_C(glDrawArrays(
                 GL_POINTS, 0, m_positions.size()));
    }

};

Cloth* cloth;


//  Update view matrix according pitch and yaw. Is called every frame.
void UpdateViewMatrix() {

    glm::mat4 cameraTransform;

    cameraTransform = glm::rotate(cameraTransform, cameraYaw, glm::vec3(0.f, 1.f, 0.f)); // add yaw
    cameraTransform = glm::rotate(cameraTransform, cameraPitch, glm::vec3(0.f, 0.f, 1.f)); // add pitch

    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    cameraPos = glm::vec3(cameraTransform * glm::vec4(cameraZoom, 0.0, 0.0, 1.0));

    viewMatrix = glm::lookAt(
        cameraPos,
        center,
        up
        );
}

void InitGlfw() {
    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "TESS", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    // load GLAD.
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Bind and create VAO, otherwise, we can't do anything in OpenGL.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GL_C(glDisable(GL_CULL_FACE));
    GL_C(glEnable(GL_DEPTH_TEST));
}

void Render() {
    int fbWidth, fbHeight;
    int wWidth, wHeight;

    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glfwGetWindowSize(window, &wWidth, &wHeight);

    // important that we do this, otherwise it won't work on retina!
    float ratio = fbWidth / (float)wWidth; //
    int s = ratio * GUI_WIDTH;

    // a tiny left part of the window is dedicated to GUI. So shift the viewport to the right some.
    GL_C(glViewport(s, 0, fbWidth - s, fbHeight));
    GL_C(glClearColor(0.0f, 0.0f, 0.3f, 1.0f));
    GL_C(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // update matrices.
    UpdateViewMatrix();
    glm::mat4 MVP = projectionMatrix * viewMatrix;

    GLuint shader;

    shader = normalShader;

    GL_C(glUseProgram(shader));
    GL_C(glUniformMatrix4fv(glGetUniformLocation(shader, "uMvp"), 1, GL_FALSE, glm::value_ptr(MVP)));
    GL_C(glUniformMatrix4fv(glGetUniformLocation(shader, "uView"), 1, GL_FALSE, glm::value_ptr(viewMatrix)));

    cloth->Render();

    // render GUI
    {
        ImGui::SetNextWindowSize(ImVec2(GUI_WIDTH, WINDOW_HEIGHT));
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0, 0.0, 0.0, 1.0)); // make non-transparent window.
        ImGui::Begin("GUI", NULL,
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse);
        {


            static float f = 0.0f;

            ImGui::Button("hello world");



        }
        ImGui::End();
        ImGui::PopStyleColor();
    }
    ImGui::Render();
}

void HandleInput() {
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeysDown[GLFW_KEY_ESCAPE]) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (!io.WantCaptureMouse) { // if not interacting with ImGui, we handle our own input.

        // zoom with mouse-wheel.
        cameraZoom += GetMouseWheel();

        prevMouseX = curMouseX;
        prevMouseY = curMouseY;
        glfwGetCursorPos(window, &curMouseX, &curMouseY);

        const float MOUSE_SENSITIVITY = 0.005;

        int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

        // we change yaw and pitch by dragging with the mouse.
        if (state == GLFW_PRESS) {
            cameraYaw += (curMouseX - prevMouseX) * MOUSE_SENSITIVITY;
            cameraPitch += (curMouseY - prevMouseY) * MOUSE_SENSITIVITY;
        }
    }
}


int main(int argc, char** argv)
{

    InitGlfw();

    // init ImGui
    ImGui_ImplGlfwGL3_Init(window, true);

    normalShader = LoadNormalShader(LoadFile("simple.vs"), LoadFile("simple.fs"));

    // setup projection matrix.
    projectionMatrix = glm::perspective(0.9f, (float)(WINDOW_WIDTH - GUI_WIDTH) / WINDOW_HEIGHT, 0.1f, 1000.0f);

    cloth = new Cloth();

    FpsManager fpsManager(60);

    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        float delta = fpsManager.ManageFPS();

        cloth->Update(delta);

        Render();

        HandleInput();

        // set window title.
        string windowTitle = "FPS: " + fpsManager.GetFpsString( ) + " delta " + std::to_string(delta);
        glfwSetWindowTitle(window, windowTitle.c_str());

        /* display and process events through callbacks */
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
