#include "fps_manager.hpp"

#include <chrono>
#include <thread>

#include <GLFW/glfw3.h>

std::chrono::milliseconds oneMilliSecond(1);

FpsManager::FpsManager(const int maxFPS) : m_maxFPS(maxFPS), m_targetFrameDuration(1.0f / m_maxFPS) {

    m_frameStartTime = (float)glfwGetTime();
    m_lastReportTime = m_frameStartTime;
    m_frameCount = 0;
}


float FpsManager::ManageFPS() {

    m_frameEndTime = (float)glfwGetTime();

    float frameDuration = m_frameEndTime - m_frameStartTime;

    if ((m_frameEndTime - m_lastReportTime) > 1.0f) {
        m_lastReportTime = m_frameEndTime;

        fpsString = "fps: " + std::to_string((float)m_frameCount);
        m_frameCount = 1;

    }
    else {
        ++m_frameCount;
    }

    const float sleepDuration = m_targetFrameDuration - frameDuration;

    if (sleepDuration > 0.0) {
        std::this_thread::sleep_for(std::chrono::milliseconds((int)sleepDuration));
    }

    m_frameStartTime = (float)glfwGetTime();

    return frameDuration + (m_frameStartTime - m_frameEndTime);
}

std::string FpsManager::GetFpsString() {
    return fpsString;
}
