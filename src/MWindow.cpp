#include "MWindow.h"

#include <SDL_vulkan.h>

#include <iostream>

using namespace moo;

MWindow::MWindow(std::string title, int w, int h) : 
    m_window{nullptr}, m_windowTitle{title}, m_width{w}, m_height{h}, 
    m_quit{false}, m_framebufferResized{false}, m_minimized{false}, m_fullscreen{false}
{
    init();
}

MWindow::~MWindow() {
    if(!m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    SDL_Quit();
}

void MWindow::init() {
    Uint32 subsystems = SDL_INIT_VIDEO;
    if(SDL_Init(subsystems) < 0) {
        std::string error = SDL_GetError();			
	    throw std::runtime_error("Failed to initialize SDL: " + error);    
    }

    Uint32 options = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    m_window = SDL_CreateWindow(
        m_windowTitle.c_str(),
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        m_width, m_height, options
    );

    if(!m_window) {
        std::string error = SDL_GetError();
        throw std::runtime_error("Failed to create SDL window: " + error);
    }

    // limits resizable window to {w, h}:
    // avoid {0, 0} resizing that causes swapchain recreation error: 
    // 0 imageView dimension triggers validation layer error
    SDL_SetWindowMinimumSize(m_window, 256, 144);
    SDL_AddEventWatch(resizingEventWatcher, m_window);
}

void MWindow::handleEvent(SDL_Event& e) {
    switch (e.type) 
    {
    case SDL_QUIT:
        //close the window when user alt-f4s or clicks the X button			
        m_quit = true;
        break;
    case SDL_WINDOWEVENT:
        switch (e.window.event) 
        {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            break;
        case SDL_WINDOWEVENT_RESIZED:
            m_width =  e.window.data1;  
            m_height = e.window.data2;
            m_framebufferResized = true;
            std::cout << "resized\n";
            break; 
        case SDL_WINDOWEVENT_MINIMIZED:
            m_minimized = true;
            std::cout << "minimized\n";
            break;
        case SDL_WINDOWEVENT_MAXIMIZED:
            m_minimized = false;
            std::cout << "maximized\n";
            break;
        case SDL_WINDOWEVENT_RESTORED:
            m_minimized = false;
            std::cout << "restored\n";
            break;

        default:
            break;
        }
    case SDL_KEYDOWN:
        switch (e.key.keysym.sym)
        {
        case SDLK_RETURN:
            if(m_fullscreen) {
                SDL_SetWindowFullscreen(m_window, SDL_FALSE);
                m_fullscreen = false;
            } else {
                SDL_SetWindowFullscreen(m_window, SDL_TRUE);
                m_fullscreen = true;
                m_minimized = false;
            }
            break;
        
        default:
            break;
        }

    default:
        break;
    }
}

void MWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if(!SDL_Vulkan_CreateSurface(m_window, instance, surface)) {
        std::string error = SDL_GetError();			
	    throw std::runtime_error("SDL could not initialize Vulkan Surface. SDL Error: " + error);
    }
}

int MWindow::resizingEventWatcher(void* data, SDL_Event* event) {
    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_RESIZED) {
        SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
        if (win == (SDL_Window*)data) {
            if (event->window.data1 < 2 || event->window.data2 < 2) {
                std::cout << "width: " << event->window.data1 << "; height: " << event->window.data2 << "\n";
            }
            std::cout << "resizing.....\n";
        }
    }
    return 0;
}