#include "UIPipeline.hpp"
#include "backend/VulkanContext.hpp"
#include "imguizmo/ImGuizmo.h"
#include "glm/gtx/string_cast.hpp"
#include "renderer/Renderer.hpp"

// allocates a seperate custom descriptor pool for imgui
static void CreateImGuiDescriptorPool(VkDevice logicalDevice, VkDescriptorPool& descriptorPool)
{
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 32,
        .poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes),
        .pPoolSizes = pool_sizes
    };

    VulkanContext::VK(vkCreateDescriptorPool(logicalDevice, &pool_info, nullptr, &descriptorPool));
}

UIPipeline::UIPipeline()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    m_Context = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    SetTheme();

    // Setup Platform/Renderer backends
    GLFWwindow* window = Window::Get()->nativeWindow;
    ImGui_ImplGlfw_InitForVulkan(window, true);

    // render pass with no depth 
    s_Renderpass = std::make_unique<Renderpass>();
}

UIPipeline::~UIPipeline()
{
    VulkanContext::Get()->WaitIdle();

    DestroyFrameBuffers();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(VulkanContext::GetDevice(), m_DescriptorPool, nullptr);
    }
}

void UIPipeline::Render(const Scene* scene, const CommandBuffer& commandBuffer)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void UIPipeline::BeginRenderPass(const CommandBuffer& commandBuffer)
{
    s_Renderpass->Begin(commandBuffer, m_FrameBuffers[CURR_FRAME]);
}

void UIPipeline::EndRenderPass(const CommandBuffer& commandBuffer)
{
    s_Renderpass->End(commandBuffer);
}

void UIPipeline::AppendDebugImage(Image* image, const std::string& name)
{
    AppendDebugImage(image->getSampler(), image->getView(), image->getLayout(), name);
}

void UIPipeline::AppendDebugImage(VkSampler sampler, VkImageView view, VkImageLayout layout, const std::string& name)
{
    m_DebugImages.push_back(std::make_pair(
        ImGui_ImplVulkan_AddTexture(sampler, view, layout),
        name));
}

void UIPipeline::CreateRenderPass()
{
    s_Renderpass->CreateRenderPass(
        { VulkanContext::Get()->getSurface(0)->getFormat().format },
        VK_FORMAT_UNDEFINED, /* no depth*/
        1, false, false);

    s_Renderpass->SetClearValues({
        {.color = { { 0.0f, 0.0f, 0.0f, 1.0f } } },  // Clear color to black
    });
}

void UIPipeline::CreatePipeline()
{
    VulkanContext* context = VulkanContext::Get();
    auto logicalDevice = VulkanContext::GetDevice();

    CreateImGuiDescriptorPool(logicalDevice, m_DescriptorPool);

    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance = *(context->getInstance()),
        .PhysicalDevice = context->getPhysicalDevice()->getPhysicalDevice(),
        .Device = logicalDevice,
        .QueueFamily = context->getLogicalDevice()->getGraphicsFamily(),
        .Queue = context->getLogicalDevice()->getGraphicsQueue(),
        .DescriptorPool = m_DescriptorPool,
        .RenderPass = s_Renderpass->renderpass,
        .MinImageCount = 2,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .PipelineCache = context->getPipelineCache(),
        .Subpass = 0,
        .Allocator = nullptr,
        .CheckVkResultFn = VulkanContext::VK,
    };

    ImGui_ImplVulkan_Init(&init_info);
}

void UIPipeline::Rebuild()
{
    DestroyFrameBuffers();

    const SwapChain* swapchain = VulkanContext::Get()->getSwapChain(0);

    //Make framebuffers for each swapchain image:
    m_FrameBuffers.assign(swapchain->getImageViews().size(), VK_NULL_HANDLE);
    for (size_t i = 0; i < swapchain->getImageViews().size(); ++i)
    {
        std::vector<VkImageView> attachments;
        attachments.emplace_back(swapchain->getImageViews()[i]);

        VkFramebufferCreateInfo create_info
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = s_Renderpass->renderpass,
            .attachmentCount = uint32_t(attachments.size()),
            .pAttachments = attachments.data(),
            .width = swapchain->getExtent().width,
            .height = swapchain->getExtent().height,
            .layers = 1,
        };

        VulkanContext::VK(
            vkCreateFramebuffer(VulkanContext::GetDevice(), &create_info, nullptr, &m_FrameBuffers[i]),
            "[vulkan] Creating frame buffer failed"
        );
    }

    DestroyDebugImageDescriptors();
}

void UIPipeline::Update(const Scene* scene)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void UIPipeline::FinalizeUI()
{
    for (Layer* layer : Application::Get().GetLayerStack())
        layer->OnImGuiRender();

    for (Layer* layer : Application::Get().GetLayerStack())
        layer->OnViewportRender();

    static int selectedOption = 0; // Index of selected option

    // render a bunch of debug images
    if (!m_DebugImages.empty())
    {
        if (ImGui::Begin("Debug Viewport")) {
            // Dropdown for choosing a debug image by name
            if (ImGui::BeginCombo("Select Image", m_DebugImages[selectedOption].second.c_str())) {
                for (int i = 0; i < m_DebugImages.size(); i++) {
                    bool isSelected = (selectedOption == i);
                    if (ImGui::Selectable(m_DebugImages[i].second.c_str(), isSelected)) {
                        selectedOption = i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // Display the selected debug image
            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            ImGui::Image(m_DebugImages[selectedOption].first, viewportPanelSize);

            ImGui::End();
        }
    }

    ImGui::Render();

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void UIPipeline::DestroyFrameBuffers()
{
    for (VkFramebuffer& framebuffer : m_FrameBuffers)
    {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(VulkanContext::GetDevice(), framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        }
    }
    m_FrameBuffers.clear();
}

void UIPipeline::SetTheme()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.80f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8.00f, 8.00f);
    style.FramePadding = ImVec2(5.00f, 2.00f);
    style.CellPadding = ImVec2(6.00f, 6.00f);
    style.ItemSpacing = ImVec2(6.00f, 6.00f);
    style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
    style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
    style.IndentSpacing = 25;
    style.ScrollbarSize = 15;
    style.GrabMinSize = 10;
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 1;
    style.TabBorderSize = 1;
    style.WindowRounding = 7;
    style.ChildRounding = 4;
    style.FrameRounding = 3;
    style.PopupRounding = 4;
    style.ScrollbarRounding = 9;
    style.GrabRounding = 3;
    style.LogSliderDeadzone = 4;
    style.TabRounding = 4;

    // Load Fonts
    const char* fileName = Files::Path("../fonts/SourceSans3-Regular.ttf").c_str();
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(fileName, 24.0);
    IM_ASSERT(font != nullptr);
}

void UIPipeline::DestroyDebugImageDescriptors()
{
    for (auto& debugImage : m_DebugImages)
    {
        ImGui_ImplVulkan_RemoveTexture(debugImage.first);
    }

    m_DebugImages.clear();
}
