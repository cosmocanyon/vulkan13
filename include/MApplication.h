#pragma once

#include "MWindow.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanSwapchain.h"
#include "VulkanModel.h"

#include <memory>

namespace moo {

class MApplication {
public:
    static constexpr int WIDTH = 1024;
    static constexpr int HEIGHT = 768;

private:
    MWindow m_window{"I'm Mopugno", WIDTH, HEIGHT};
    VulkanDevice m_device{m_window};
    std::unique_ptr<VulkanSwapchain> m_swapchain;
    std::unique_ptr<VulkanPipeline> m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    std::vector<VkCommandBuffer> m_commandBuffers;
    
    std::unique_ptr<VulkanModel> model1;

public:
    MApplication();
    ~MApplication();

    MApplication(const MApplication&) = delete;
    MApplication& operator=(const MApplication&) = delete;

    void run();

private:
    void createPipelineLayout();
    void createPipeline();
    void createCommandBuffers();
    void freeCommandBuffers();
    void drawFrame();
    void reCreateSwapchain();
    void recordCommandBuffer(int imgIndex);

    void loadModels();
};

}   // namespace moo