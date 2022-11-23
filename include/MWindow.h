#pragma once

#include <SDL.h>
#include "vk_types.h"

#include <string>

namespace moo {

class MWindow {
public:
    MWindow(std::string title, int w, int h);
    ~MWindow();

    MWindow(const MWindow&) = delete; // copy constructor
    MWindow &operator=(const MWindow&) = delete;// copy operator

    void handleEvent(SDL_Event& event);
    void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

    inline VkExtent2D getExtent() { return {static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)}; }
    inline bool isMinimized() { return m_minimized; }
    inline bool wasWindowResized() { return m_framebufferResized; }
    inline void resetWindowResizedFlag() { m_framebufferResized = false; }
    inline bool isClosing() { return m_quit; }

    static int resizingEventWatcher(void* data, SDL_Event* event);

private:
    SDL_Window* m_window;
    std::string m_windowTitle;
    int m_width, m_height;
    bool m_quit, m_framebufferResized, m_fullscreen, m_minimized;

    void init();
};

}   // namespace moo