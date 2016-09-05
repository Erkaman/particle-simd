#pragma once

#include <string>

class FpsManager  {

private:

    const int m_maxFPS;
    const float m_targetFrameDuration;
    float m_frameStartTime;
    float m_frameEndTime;
    float m_lastReportTime;
    int m_frameCount;
    std::string fpsString;

public:

    FpsManager(const int maxFPS=30);

	FpsManager(const FpsManager&) = delete;
	FpsManager& operator=(const FpsManager&) = delete;

    // should be called at the start of every frame.
    float ManageFPS();

    std::string GetFpsString();

};
