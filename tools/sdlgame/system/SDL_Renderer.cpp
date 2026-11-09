#include "SDL_Renderer.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cassert>
#include <memory>
#include <type_traits>

#include "../../../a51lib/Bitmap.h"
#include "../../../a51lib/PlatformDef.h"
#include "../../../a51lib/view/View.h"

SDLRenderer::SDLRenderer()
{
    worldToView.Identity();
    viewToClip.Identity();
}

SDL_GPUShader* LoadShader(
    SDL_GPUDevice* device,
    const char*    basePath,
    const char*    shaderFilename,
    Uint32         samplerCount,
    Uint32         uniformBufferCount,
    Uint32         storageBufferCount,
    Uint32         storageTextureCount)
{
    // Auto-detect the shader stage from the file name for convenience
    SDL_GPUShaderStage stage;
    if (SDL_strstr(shaderFilename, ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if (SDL_strstr(shaderFilename, ".frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        SDL_Log("Invalid shader stage!");
        return nullptr;
    }

    char                fullPath[256];
    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char*         entrypoint;

    if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
        SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/shaders/compiled/SPIRV/%s.spv", basePath, shaderFilename);
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    } else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
        SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/shaders/compiled/MSL/%s.msl", basePath, shaderFilename);
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
    } else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
        SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/shaders/compiled/DXIL/%s.dxil", basePath, shaderFilename);
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
    } else {
        SDL_Log("%s", "Unrecognized backend shader format!");
        return nullptr;
    }

    size_t codeSize;
    void*  code = SDL_LoadFile(fullPath, &codeSize);
    if (code == nullptr) {
        SDL_Log("Failed to load shader from disk! %s", fullPath);
        return nullptr;
    }

    SDL_GPUShaderCreateInfo shaderInfo = {
        .code_size = codeSize,
        .code = (const Uint8*)code,
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = samplerCount,
        .num_storage_textures = storageTextureCount,
        .num_storage_buffers = storageBufferCount,
        .num_uniform_buffers = uniformBufferCount};
    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
    if (shader == nullptr) {
        SDL_Log("Failed to create shader!");
        SDL_free(code);
        return nullptr;
    }

    SDL_free(code);
    return shader;
}

bool SDLRenderer::init()
{
    device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
        false,
        nullptr);

    if (device == nullptr) {
        SDL_Log("GPUCreateDevice failed");
        return false;
    }

    window = SDL_CreateWindow("SDL View", 640, 480, 0);
    if (window == nullptr) {
        SDL_Log("CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("GPUClaimWindow failed");
        return false;
    }

    const char* basePath = SDL_GetBasePath();

    SDL_GPUShader* vertexShader = LoadShader(device, basePath, "Textured.vert", 0, 0, 0, 0);
    if (vertexShader == nullptr) {
        SDL_Log("Failed to create vertex shader!");
        return false;
    }

    SDL_GPUShader* fragmentShader = LoadShader(device, basePath, "Textured.frag", 1, 0, 0, 0);
    if (fragmentShader == nullptr) {
        SDL_Log("Failed to create fragment shader!");
        return false;
    }

    SDL_GPUShader* positionColorVertexShader = LoadShader(device, basePath, "PositionColor.vert", 0, 0, 0, 0);
    if (vertexShader == nullptr) {
        SDL_Log("Failed to create vertex shader!");
        return false;
    }

    SDL_GPUShader* solidColorFragmentShader = LoadShader(device, basePath, "SolidColor.frag", 0, 0, 0, 0);
    if (fragmentShader == nullptr) {
        SDL_Log("Failed to create fragment shader!");
        return false;
    }
    SDL_GPUColorTargetDescription ctd_add[]{
        {.format = SDL_GetGPUSwapchainTextureFormat(device, window),
         .blend_state = {
             .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
             .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
             .color_blend_op = SDL_GPU_BLENDOP_ADD,
             .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
             .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
             .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
             .color_write_mask = 0,
             .enable_blend = true,
             .enable_color_write_mask = false}}};

    SDL_GPUColorTargetDescription ctd_opaque[]{
        {.format = SDL_GetGPUSwapchainTextureFormat(device, window),
         .blend_state = {.enable_blend = false}}};

    SDL_GPUColorTargetDescription ctd_alpha[]{
        {.format = SDL_GetGPUSwapchainTextureFormat(device, window),
         .blend_state = {
             .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
             .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
             .color_blend_op = SDL_GPU_BLENDOP_ADD,
             .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
             .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
             .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
             .color_write_mask = 0,
             .enable_blend = true,
             .enable_color_write_mask = false}}};

    SDL_GPUVertexAttribute vba[]{
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = 0,
        },
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = sizeof(float) * 4,
        },
        {
            .location = 2,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = sizeof(float) * 6,
        }};

    SDL_GPUVertexBufferDescription vbd[]{
        {
            .slot = 0,
            .pitch = sizeof(PositionTextureVertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0,
        }};

    SDL_GPUVertexInputState vertexInputState{
        .vertex_buffer_descriptions = vbd,
        .num_vertex_buffers = 1,
        .vertex_attributes = vba,
        .num_vertex_attributes = 3,
    };

    SDL_GPUVertexAttribute positionColourVba[]{
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = 0,
        },
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = sizeof(float) * 4,
        }};

    SDL_GPUVertexBufferDescription positionColourVbd[]{
        {
            .slot = 0,
            .pitch = sizeof(PositionColourVertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0,
        }};

    SDL_GPUVertexInputState positionColourVertexInputState{
        .vertex_buffer_descriptions = positionColourVbd,
        .num_vertex_buffers = 1,
        .vertex_attributes = positionColourVba,
        .num_vertex_attributes = 2,
    };

    SDL_GPURasterizerState rasteriserState{
        .fill_mode = SDL_GPU_FILLMODE_FILL,
        .cull_mode = SDL_GPU_CULLMODE_NONE,
        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        .depth_bias_constant_factor = 0.0f,
        .depth_bias_clamp = 0.0f,
        .depth_bias_slope_factor = 0.0f,
        .enable_depth_bias = false,
        .enable_depth_clip = false,
    };

    SDL_GPUMultisampleState multisampleState{
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
        .sample_mask = 0xFFFFFFFF,
        .enable_mask = false,
    };

    SDL_GPUDepthStencilState depthStencilState{
        .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
        .back_stencil_state = {
            .fail_op = SDL_GPU_STENCILOP_INVALID,
            .pass_op = SDL_GPU_STENCILOP_INVALID,
            .depth_fail_op = SDL_GPU_STENCILOP_INVALID,
            .compare_op = SDL_GPU_COMPAREOP_INVALID,
        },
        .front_stencil_state = {
            .fail_op = SDL_GPU_STENCILOP_INVALID,
            .pass_op = SDL_GPU_STENCILOP_INVALID,
            .depth_fail_op = SDL_GPU_STENCILOP_INVALID,
            .compare_op = SDL_GPU_COMPAREOP_INVALID,
        },
        .compare_mask = 0,
        .write_mask = 0,
        .enable_depth_test = true,
        .enable_depth_write = true,
        .enable_stencil_test = false,
    };

    constexpr SDL_GPUTextureFormat depthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    const auto createPipeline = [this, &rasteriserState, &multisampleState, &depthStencilState, depthFormat](
                                    SDL_GPUShader* vertex,
                                    SDL_GPUShader* fragment,
                                    const SDL_GPUVertexInputState& vertexInput,
                                    SDL_GPUPrimitiveType primitiveType,
                                    SDL_GPUColorTargetDescription* colorTarget,
                                    bool enableDepthWrite) {
        SDL_GPUDepthStencilState pipelineDepthState = depthStencilState;
        pipelineDepthState.enable_depth_write = enableDepthWrite;
        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{
            .vertex_shader = vertex,
            .fragment_shader = fragment,
            .vertex_input_state = vertexInput,
            .primitive_type = primitiveType,
            .rasterizer_state = rasteriserState,
            .multisample_state = multisampleState,
            .depth_stencil_state = pipelineDepthState,
            .target_info = {
                .color_target_descriptions = colorTarget,
                .num_color_targets = 1,
                .depth_stencil_format = depthFormat,
                .has_depth_stencil_target = true,
            },
            .props = 0,
        };
        return SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);
    };

    pipeline = createPipeline(vertexShader, fragmentShader, vertexInputState,
                              SDL_GPU_PRIMITIVETYPE_TRIANGLELIST, ctd_add, true);
    pipeline_no_depth_write = createPipeline(vertexShader, fragmentShader, vertexInputState,
                                             SDL_GPU_PRIMITIVETYPE_TRIANGLELIST, ctd_add, false);
    rigidOpaquePipeline = createPipeline(vertexShader, fragmentShader, vertexInputState,
                                         SDL_GPU_PRIMITIVETYPE_TRIANGLELIST, ctd_opaque, true);
    rigidOpaquePipeline_no_depth_write = createPipeline(vertexShader, fragmentShader, vertexInputState,
                                                        SDL_GPU_PRIMITIVETYPE_TRIANGLELIST, ctd_opaque, false);
    pipeline_tex_alpha = createPipeline(vertexShader, fragmentShader, vertexInputState,
                                        SDL_GPU_PRIMITIVETYPE_TRIANGLELIST, ctd_alpha, true);
    pipeline_tex_alpha_no_depth_write = createPipeline(vertexShader, fragmentShader, vertexInputState,
                                                       SDL_GPU_PRIMITIVETYPE_TRIANGLELIST, ctd_alpha, false);
    colourPipeline = createPipeline(positionColorVertexShader, solidColorFragmentShader,
                                    positionColourVertexInputState, SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                                    ctd_add, true);
    colourPipeline_no_depth_write = createPipeline(positionColorVertexShader, solidColorFragmentShader,
                                                   positionColourVertexInputState, SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                                                   ctd_add, false);
    rigidOpaqueColourPipeline = createPipeline(positionColorVertexShader, solidColorFragmentShader,
                                               positionColourVertexInputState, SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                                               ctd_opaque, true);
    rigidOpaqueColourPipeline_no_depth_write = createPipeline(positionColorVertexShader, solidColorFragmentShader,
                                                              positionColourVertexInputState, SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                                                              ctd_opaque, false);
    colourPipeline_alpha = createPipeline(positionColorVertexShader, solidColorFragmentShader,
                                          positionColourVertexInputState, SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                                          ctd_alpha, true);
    colourPipeline_alpha_no_depth_write = createPipeline(positionColorVertexShader, solidColorFragmentShader,
                                                         positionColourVertexInputState, SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                                                         ctd_alpha, false);
    wireframePipeline = createPipeline(positionColorVertexShader, solidColorFragmentShader,
                                       positionColourVertexInputState, SDL_GPU_PRIMITIVETYPE_LINELIST,
                                       ctd_add, true);
    wireframePipeline_no_depth_write = createPipeline(positionColorVertexShader, solidColorFragmentShader,
                                                      positionColourVertexInputState, SDL_GPU_PRIMITIVETYPE_LINELIST,
                                                      ctd_add, false);
    if (pipeline == nullptr || pipeline_no_depth_write == nullptr || rigidOpaquePipeline == nullptr ||
        rigidOpaquePipeline_no_depth_write == nullptr || pipeline_tex_alpha == nullptr ||
        pipeline_tex_alpha_no_depth_write == nullptr || colourPipeline == nullptr ||
        colourPipeline_no_depth_write == nullptr || rigidOpaqueColourPipeline == nullptr ||
        rigidOpaqueColourPipeline_no_depth_write == nullptr || colourPipeline_alpha == nullptr ||
        colourPipeline_alpha_no_depth_write == nullptr || wireframePipeline == nullptr ||
        wireframePipeline_no_depth_write == nullptr) {
        SDL_Log("Failed to create a depth-enabled graphics pipeline: %s", SDL_GetError());
        return false;
    }
    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);
    SDL_ReleaseGPUShader(device, positionColorVertexShader);
    SDL_ReleaseGPUShader(device, solidColorFragmentShader);

    SDL_GPUSamplerCreateInfo sci{
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    pointSampler = SDL_CreateGPUSampler(device, &sci);

    return true;
}

void SDLRenderer::setView(const view& View)
{
    worldToView = View.GetW2V();
    viewToClip = View.GetV2C();
}

void SDLRenderer::quit()
{
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    pipeline = nullptr;

    SDL_ReleaseGPUGraphicsPipeline(device, pipeline_no_depth_write);
    pipeline_no_depth_write = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, rigidOpaquePipeline);
    rigidOpaquePipeline = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, rigidOpaquePipeline_no_depth_write);
    rigidOpaquePipeline_no_depth_write = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline_tex_alpha);
    pipeline_tex_alpha = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline_tex_alpha_no_depth_write);
    pipeline_tex_alpha_no_depth_write = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, colourPipeline);
    colourPipeline = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, colourPipeline_no_depth_write);
    colourPipeline_no_depth_write = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, rigidOpaqueColourPipeline);
    rigidOpaqueColourPipeline = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, rigidOpaqueColourPipeline_no_depth_write);
    rigidOpaqueColourPipeline_no_depth_write = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, colourPipeline_alpha);
    colourPipeline_alpha = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, colourPipeline_alpha_no_depth_write);
    colourPipeline_alpha_no_depth_write = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, wireframePipeline);
    wireframePipeline = nullptr;
    SDL_ReleaseGPUGraphicsPipeline(device, wireframePipeline_no_depth_write);
    wireframePipeline_no_depth_write = nullptr;

    for (const auto& [key, value] : gpuTextures) {
        SDL_ReleaseGPUTexture(device, value);
    }

    gpuTextures.clear();

    if (depthTexture != nullptr) {
        SDL_ReleaseGPUTexture(device, depthTexture);
        depthTexture = nullptr;
    }

    SDL_ReleaseGPUSampler(device, pointSampler);

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyWindow(window);
    SDL_DestroyGPUDevice(device);
}

void SDLRenderer::draw()
{
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(device);
    if (cmdbuf == nullptr) {
        SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchainTexture;
    Uint32 swapchainWidth;
    Uint32 swapchainHeight;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            cmdbuf, window, &swapchainTexture, &swapchainWidth, &swapchainHeight)) {
        SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(cmdbuf);
        return;
    }

    if (swapchainTexture != nullptr) {
        if (depthTexture == nullptr || depthTextureWidth != swapchainWidth ||
            depthTextureHeight != swapchainHeight) {
            SDL_GPUTextureCreateInfo depthTextureInfo{
                .type = SDL_GPU_TEXTURETYPE_2D,
                .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
                .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
                .width = swapchainWidth,
                .height = swapchainHeight,
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
            };
            SDL_GPUTexture* newDepthTexture = SDL_CreateGPUTexture(device, &depthTextureInfo);
            if (newDepthTexture == nullptr) {
                SDL_Log("Failed to create depth texture: %s", SDL_GetError());
                SDL_CancelGPUCommandBuffer(cmdbuf);
                return;
            }
            if (depthTexture != nullptr) {
                SDL_ReleaseGPUTexture(device, depthTexture);
            }
            depthTexture = newDepthTexture;
            depthTextureWidth = swapchainWidth;
            depthTextureHeight = swapchainHeight;
        }

        SDL_FColor             black{0.0f, 0.0f, 0.0f, 1.0f};
        SDL_GPUColorTargetInfo colorTargetInfo = {0};
        colorTargetInfo.texture = swapchainTexture;
        colorTargetInfo.clear_color = black;
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPUDepthStencilTargetInfo depthTargetInfo{
            .texture = depthTexture,
            .clear_depth = 1.0f,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
            .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
            .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        };
        SDL_GPURenderPass* renderPass =
            SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, &depthTargetInfo);

        for (auto& batch : batches) {
            SDL_BindGPUGraphicsPipeline(renderPass, batch.pipeline);
            SDL_GPUBufferBinding bb{.buffer = batch.vertexBuffer, .offset = 0};
            SDL_BindGPUVertexBuffers(renderPass, 0, &bb, 1);
            SDL_GPUBufferBinding bbi{.buffer = batch.indexBuffer, .offset = 0};
            SDL_BindGPUIndexBuffer(renderPass, &bbi, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            if (batch.texture != nullptr) {
                SDL_GPUTextureSamplerBinding tsb{.texture = batch.texture, .sampler = pointSampler};
                SDL_BindGPUFragmentSamplers(renderPass, 0, &tsb, 1);
            }
            SDL_DrawGPUIndexedPrimitives(renderPass, batch.numIndices, 1, 0, 0, 0);

            SDL_ReleaseGPUBuffer(device, batch.vertexBuffer);
            SDL_ReleaseGPUBuffer(device, batch.indexBuffer);
        }

        SDL_EndGPURenderPass(renderPass);
    }
    batches.clear();
    SDL_SubmitGPUCommandBuffer(cmdbuf);
}

void SDLRenderer::drawBegin(Primitive prim, int drawFlags)
{
    primitive = prim;
    this->drawFlags = drawFlags;
    is2D = drawFlags & (DRAW_2D | DRAW_2D_KEEP_Z);
    isTextured = drawFlags & DRAW_TEXTURED;

    if (prim == Primitive::DRAW_TRIANGLES) {
    }
}

void SDLRenderer::drawColour(const Colour& colour)
{
    currentColour = colour;
}

void SDLRenderer::drawVertex(float x, float y, float z, float u, float v)
{
    float   wx = (x - 320.0f) / 320.0f;
    float   wy = (240.0f - y) / 240.0f;
    Colour& c = currentColour;
    accumulatedVertices.push_back({wx, wy, z, 1.0f, u, v, c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f});
}

void SDLRenderer::drawColourVertex(float x, float y, float z, const Colour& c)
{
    float wx = (x - 320.0f) / 320.0f;
    float wy = (240.0f - y) / 240.0f;
    accumulatedColourVertices.push_back({wx, wy, z, 1.0f, c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f});
}

void SDLRenderer::drawSpriteUV(const Vector3& position, // Hot spot (2D Left-Top), (3D Center)
                               const Vector2& WH,       // (2D pixel W&H), (3D World W&H)
                               const Vector2& UV0,      // Upper Left   UV  [0.0 - 1.0]
                               const Vector2& UV1,      // Bottom Right UV  [0.0 - 1.0]
                               const Colour&  colour)
{
    if (colour != currentColour) {
        drawEnd();
        currentColour = colour;
        drawBegin(primitive, drawFlags);
    }

    if (is2D) {
        IntRect r(position.x, position.y, position.x + WH.x, position.y + WH.y);
        drawVertex(r.left, r.top, 0.5f, UV0.x, UV0.y);
        drawVertex(r.right, r.top, 0.5f, UV1.x, UV0.y);
        drawVertex(r.left, r.bottom, 0.5f, UV0.x, UV1.y);

        drawVertex(r.left, r.bottom, 0.5f, UV0.x, UV1.y);
        drawVertex(r.right, r.bottom, 0.5f, UV1.x, UV1.y);
        drawVertex(r.right, r.top, 0.5f, UV1.x, UV0.y);

    } else {
        assert(false);
    }
}

void SDLRenderer::drawColourRect(const IntRect& pos, const Colour& colour, bool isAdditive)
{
    drawColourVertex(pos.left, pos.top, 0.5f, colour);
    drawColourVertex(pos.right, pos.top, 0.5f, colour);
    drawColourVertex(pos.left, pos.bottom, 0.5f, colour);

    drawColourVertex(pos.left, pos.bottom, 0.5f, colour);
    drawColourVertex(pos.right, pos.bottom, 0.5f, colour);
    drawColourVertex(pos.right, pos.top, 0.5f, colour);
}

void SDLRenderer::flushTextureVertices()
{
    if (accumulatedVertices.empty()) {
        return;
    }

    SDL_GPUBufferCreateInfo vbci{
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = (Uint32)(sizeof(PositionTextureVertex) * accumulatedVertices.size())};
    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(
        device,
        &vbci);

    SDL_GPUBufferCreateInfo ibci{
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = (Uint32)(sizeof(Uint16) * accumulatedVertices.size())};
    SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(
        device,
        &ibci);

    SDL_GPUTransferBufferCreateInfo tbci{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = (Uint32)((sizeof(PositionTextureVertex) + sizeof(Uint16)) * accumulatedVertices.size())};

    SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(
        device,
        &tbci);

    PositionTextureVertex* transferData = (PositionTextureVertex*)SDL_MapGPUTransferBuffer(
        device,
        bufferTransferBuffer,
        false);
    Uint16* indexData = (Uint16*)&transferData[accumulatedVertices.size()];
    for (int i = 0; i < accumulatedVertices.size(); ++i) {
        transferData[i] = accumulatedVertices[i];
        indexData[i] = i;
    }

    SDL_UnmapGPUTransferBuffer(device, bufferTransferBuffer);

    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass*      copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

    SDL_GPUTransferBufferLocation tbl{
        .transfer_buffer = bufferTransferBuffer,
        .offset = 0};
    SDL_GPUBufferRegion br{
        .buffer = vertexBuffer,
        .offset = 0,
        .size = (Uint32)(sizeof(PositionTextureVertex) * accumulatedVertices.size())};
    SDL_UploadToGPUBuffer(
        copyPass,
        &tbl,
        &br,
        false);

    SDL_GPUTransferBufferLocation ibl{
        .transfer_buffer = bufferTransferBuffer,
        .offset = (Uint32)(sizeof(PositionTextureVertex) * accumulatedVertices.size())};
    SDL_GPUBufferRegion ibr{
        .buffer = indexBuffer,
        .offset = 0,
        .size = (Uint32)(sizeof(Uint16) * accumulatedVertices.size())};
    SDL_UploadToGPUBuffer(
        copyPass,
        &ibl,
        &ibr,
        false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuf);

    // DRAW_BLEND_ADD takes precedence
    SDL_GPUGraphicsPipeline* pl = pipeline;
    if ((drawFlags & DRAW_BLEND_ADD)) {
        pl = pipeline;
    } else if ((drawFlags & DRAW_BLEND_SUB)) {
        assert(false);
    } else if ((drawFlags & DRAW_USE_ALPHA)) {
        pl = pipeline_tex_alpha;
    }

    batches.push_back(Batch{pl, gpuTextures[currentTex], vertexBuffer, indexBuffer, (int)accumulatedVertices.size(), currentColour});

    accumulatedVertices.clear();
}

void SDLRenderer::flushColourVertices()
{
    if (accumulatedColourVertices.empty()) {
        return;
    }

    SDL_GPUBufferCreateInfo vbci{
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = (Uint32)(sizeof(PositionColourVertex) * accumulatedColourVertices.size())};
    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(
        device,
        &vbci);

    SDL_GPUBufferCreateInfo ibci{
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = (Uint32)(sizeof(Uint16) * accumulatedColourVertices.size())};
    SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(
        device,
        &ibci);

    SDL_GPUTransferBufferCreateInfo tbci{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = (Uint32)((sizeof(PositionColourVertex) + sizeof(Uint16)) * accumulatedColourVertices.size())};

    SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(
        device,
        &tbci);

    PositionColourVertex* transferData = (PositionColourVertex*)SDL_MapGPUTransferBuffer(
        device,
        bufferTransferBuffer,
        false);
    Uint16* indexData = (Uint16*)&transferData[accumulatedColourVertices.size()];
    for (int i = 0; i < accumulatedColourVertices.size(); ++i) {
        transferData[i] = accumulatedColourVertices[i];
        indexData[i] = i;
    }

    SDL_UnmapGPUTransferBuffer(device, bufferTransferBuffer);

    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass*      copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

    SDL_GPUTransferBufferLocation tbl{
        .transfer_buffer = bufferTransferBuffer,
        .offset = 0};
    SDL_GPUBufferRegion br{
        .buffer = vertexBuffer,
        .offset = 0,
        .size = (Uint32)(sizeof(PositionColourVertex) * accumulatedColourVertices.size())};
    SDL_UploadToGPUBuffer(
        copyPass,
        &tbl,
        &br,
        false);

    SDL_GPUTransferBufferLocation ibl{
        .transfer_buffer = bufferTransferBuffer,
        .offset = (Uint32)(sizeof(PositionColourVertex) * accumulatedColourVertices.size())};
    SDL_GPUBufferRegion ibr{
        .buffer = indexBuffer,
        .offset = 0,
        .size = (Uint32)(sizeof(Uint16) * accumulatedColourVertices.size())};
    SDL_UploadToGPUBuffer(
        copyPass,
        &ibl,
        &ibr,
        false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuf);

    batches.push_back(Batch{colourPipeline, nullptr, vertexBuffer, indexBuffer, (int)accumulatedColourVertices.size(), currentColour});

    accumulatedColourVertices.clear();
}

void SDLRenderer::drawEnd()
{
    flushTextureVertices();
    flushColourVertices();
}

void SDLRenderer::setTexture(Bitmap* tex)
{
    currentTex = tex;
    if (gpuTextures.contains(tex)) {
        return;
    }
    tex->convertFormat(Bitmap::FMT_32_ABGR_8888); // RGBA in memory
    SDL_GPUTextureCreateInfo ci{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = (Uint32)tex->width,
        .height = (Uint32)tex->height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    SDL_GPUTexture* Texture = SDL_CreateGPUTexture(device, &ci);

    // Set up texture data
    const Uint32 sourcePitch = static_cast<Uint32>(tex->physicalWidth);
    const Uint32 sourceSize = sourcePitch * static_cast<Uint32>(tex->height) * 4;
    SDL_GPUTransferBufferCreateInfo tbci{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = sourceSize,
    };
    SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(
        device,
        &tbci);

    Uint8* textureTransferPtr = (Uint8*)SDL_MapGPUTransferBuffer(
        device,
        textureTransferBuffer,
        false);
    SDL_memcpy(textureTransferPtr, tex->getPixelData(0), sourceSize);
    SDL_UnmapGPUTransferBuffer(device, textureTransferBuffer);

    // Upload the transfer data to the GPU resources
    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass*      copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

    SDL_GPUTextureTransferInfo tti{
        .transfer_buffer = textureTransferBuffer,
        .offset = 0,
        .pixels_per_row = sourcePitch,
        .rows_per_layer = static_cast<Uint32>(tex->height),
    };
    SDL_GPUTextureRegion tr{
        .texture = Texture,
        .w = (Uint32)tex->width,
        .h = (Uint32)tex->height,
        .d = 1};
    SDL_UploadToGPUTexture(
        copyPass,
        &tti,
        &tr,
        false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
    SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);

    gpuTextures[tex] = Texture;
}

void SDLRenderer::BeginRigidGeom(Geom* pGeom, int iSubMesh)
{
    currentRigidGeom = static_cast<RigidGeom*>(pGeom);
    currentRigidSubmesh = iSubMesh;
}

void SDLRenderer::RenderRigidInstance(render_instance& Inst)
{
    if (currentRigidGeom == nullptr || Inst.Data.Rigid.pGeom != currentRigidGeom ||
        currentRigidSubmesh < 0 || currentRigidSubmesh >= currentRigidGeom->numSubMeshes) {
        SDL_Log("RenderRigidInstance called without a matching rigid geometry");
        return;
    }

    const int numVertices = currentRigidGeom->getNumSubmeshVertices(currentRigidSubmesh);
    if (numVertices < 3) {
        return;
    }

    std::unique_ptr<float[]> puv(currentRigidGeom->getSubmeshVerticesPUV(currentRigidSubmesh));
    if (puv == nullptr) {
        SDL_Log("Failed to extract rigid geometry vertices");
        return;
    }

    const material& activeMaterial = render::GetMaterial(Inst.SortKey.MatIndex);
    const bool alphaMaterial = activeMaterial.m_Type == Material_Alpha ||
                               activeMaterial.m_Type == Material_Alpha_PerPolyEnv ||
                               activeMaterial.m_Type == Material_Alpha_PerPixelIllum ||
                               activeMaterial.m_Type == Material_Alpha_PerPolyIllum;
    const bool writesDepth = !alphaMaterial ||
                             (activeMaterial.m_Flags & Geom::Material::FLAG_FORCE_ZFILL) != 0;

    Bitmap* diffuseBitmap = nullptr;
    texture* diffuseTexture = activeMaterial.m_DiffuseMap.getPointer();
    if (diffuseTexture != nullptr) {
        diffuseBitmap = &diffuseTexture->m_Bitmap;
        setTexture(diffuseBitmap);
    }

    SDL_GPUTexture* texture = nullptr;
    if (diffuseBitmap != nullptr) {
        const auto textureIt = gpuTextures.find(diffuseBitmap);
        if (textureIt != gpuTextures.end()) {
            texture = textureIt->second;
        }
    }
    const auto* pcDlist = currentRigidGeom->platform == PLATFORM_PC
                              ? &currentRigidGeom->system.pPC[
                                    currentRigidGeom->subMeshes[currentRigidSubmesh].iDList]
                              : nullptr;

    std::vector<PositionTextureVertex> texturedVertices;
    std::vector<PositionColourVertex> colourVertices;
    std::vector<PositionColourVertex> wireVertices;
    if (texture != nullptr) {
        texturedVertices.reserve(numVertices);
    } else {
        colourVertices.reserve(numVertices);
    }
    if (wireframe) {
        wireVertices.reserve(static_cast<size_t>(numVertices) * 2);
    }

    const float instanceAlpha = Inst.Alpha / 255.0f;
    const bool usesAlpha = alphaMaterial || Inst.Alpha < 255;
    for (int i = 0; i < numVertices; ++i) {
        Vector4 position(puv[i * 3], puv[i * 3 + 1], puv[i * 3 + 2], 1.0f);
        if (Inst.Data.Rigid.pL2W != nullptr) {
            position = *Inst.Data.Rigid.pL2W * position;
        }
        position = viewToClip * (worldToView * position);
        Colour vertexColour = COLOR_WHITE;
        if (pcDlist != nullptr) {
            vertexColour = pcDlist->verts[pcDlist->indices[i]].colour;
        }
        if (texture != nullptr &&
            vertexColour.r == 0 && vertexColour.g == 0 && vertexColour.b == 0) {
            vertexColour.r = 255;
            vertexColour.g = 255;
            vertexColour.b = 255;
        }
        const float red = vertexColour.r / 255.0f;
        const float green = vertexColour.g / 255.0f;
        const float blue = vertexColour.b / 255.0f;
        const float alpha = (vertexColour.a / 255.0f) * instanceAlpha;

        if (texture != nullptr) {
            texturedVertices.push_back({
                position.x,
                position.y,
                position.z,
                position.w,
                puv[numVertices * 3 + i * 2],
                puv[numVertices * 3 + i * 2 + 1],
                red,
                green,
                blue,
                alpha});
        } else {
            colourVertices.push_back({
                position.x,
                position.y,
                position.z,
                position.w,
                red,
                green,
                blue,
                alpha});
        }
    }

    if (wireframe) {
        auto appendEdge = [&wireVertices](const auto& from, const auto& to) {
            wireVertices.push_back({from.x, from.y, from.z, from.w, from.r, from.g, from.b, from.a});
            wireVertices.push_back({to.x, to.y, to.z, to.w, to.r, to.g, to.b, to.a});
        };
        for (int i = 0; i + 2 < numVertices; i += 3) {
            if (texture != nullptr) {
                appendEdge(texturedVertices[i], texturedVertices[i + 1]);
                appendEdge(texturedVertices[i + 1], texturedVertices[i + 2]);
                appendEdge(texturedVertices[i + 2], texturedVertices[i]);
            } else {
                appendEdge(colourVertices[i], colourVertices[i + 1]);
                appendEdge(colourVertices[i + 1], colourVertices[i + 2]);
                appendEdge(colourVertices[i + 2], colourVertices[i]);
            }
        }
    }

    const size_t maxVerticesPerBatch = 65535;
    const size_t renderVertexCount = wireframe ? wireVertices.size() : static_cast<size_t>(numVertices);
    const size_t verticesPerPrimitive = wireframe ? 2 : 3;
    for (size_t firstVertex = 0; firstVertex < renderVertexCount;) {
        size_t vertexCount = std::min(maxVerticesPerBatch, renderVertexCount - firstVertex);
        vertexCount -= vertexCount % verticesPerPrimitive;
        if (vertexCount == 0) {
            SDL_Log("Rigid geometry has an incomplete primitive");
            break;
        }

        auto uploadBatch = [this, firstVertex, vertexCount](const auto& vertices,
                                                            SDL_GPUGraphicsPipeline* batchPipeline,
                                                            SDL_GPUTexture* batchTexture) {
            using Vertex = typename std::decay_t<decltype(vertices)>::value_type;
            const Uint32 vertexBytes = static_cast<Uint32>(sizeof(Vertex) * vertexCount);
            const Uint32 indexBytes = static_cast<Uint32>(sizeof(Uint16) * vertexCount);

            SDL_GPUBufferCreateInfo vbci{
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = vertexBytes};
            SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(device, &vbci);
            SDL_GPUBufferCreateInfo ibci{
                .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                .size = indexBytes};
            SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(device, &ibci);
            if (vertexBuffer == nullptr || indexBuffer == nullptr) {
                SDL_Log("Failed to create rigid geometry GPU buffers: %s", SDL_GetError());
                if (vertexBuffer != nullptr) {
                    SDL_ReleaseGPUBuffer(device, vertexBuffer);
                }
                if (indexBuffer != nullptr) {
                    SDL_ReleaseGPUBuffer(device, indexBuffer);
                }
                return false;
            }

            SDL_GPUTransferBufferCreateInfo tbci{
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = vertexBytes + indexBytes};
            SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &tbci);
            if (transferBuffer == nullptr) {
                SDL_Log("Failed to create rigid geometry transfer buffer: %s", SDL_GetError());
                SDL_ReleaseGPUBuffer(device, vertexBuffer);
                SDL_ReleaseGPUBuffer(device, indexBuffer);
                return false;
            }

            auto* transferData = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(device, transferBuffer, false));
            if (transferData == nullptr) {
                SDL_Log("Failed to map rigid geometry transfer buffer: %s", SDL_GetError());
                SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
                SDL_ReleaseGPUBuffer(device, vertexBuffer);
                SDL_ReleaseGPUBuffer(device, indexBuffer);
                return false;
            }
            SDL_memcpy(transferData, vertices.data() + firstVertex, vertexBytes);
            auto* indexData = reinterpret_cast<Uint16*>(transferData + vertexBytes);
            for (size_t i = 0; i < vertexCount; ++i) {
                indexData[i] = static_cast<Uint16>(i);
            }
            SDL_UnmapGPUTransferBuffer(device, transferBuffer);

            SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
            if (uploadCommandBuffer == nullptr) {
                SDL_Log("Failed to acquire rigid geometry upload command buffer: %s", SDL_GetError());
                SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
                SDL_ReleaseGPUBuffer(device, vertexBuffer);
                SDL_ReleaseGPUBuffer(device, indexBuffer);
                return false;
            }
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);
            if (copyPass == nullptr) {
                SDL_Log("Failed to begin rigid geometry upload pass: %s", SDL_GetError());
                SDL_CancelGPUCommandBuffer(uploadCommandBuffer);
                SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
                SDL_ReleaseGPUBuffer(device, vertexBuffer);
                SDL_ReleaseGPUBuffer(device, indexBuffer);
                return false;
            }

            SDL_GPUTransferBufferLocation vertexLocation{
                .transfer_buffer = transferBuffer,
                .offset = 0};
            SDL_GPUBufferRegion vertexRegion{
                .buffer = vertexBuffer,
                .offset = 0,
                .size = vertexBytes};
            SDL_UploadToGPUBuffer(copyPass, &vertexLocation, &vertexRegion, false);

            SDL_GPUTransferBufferLocation indexLocation{
                .transfer_buffer = transferBuffer,
                .offset = vertexBytes};
            SDL_GPUBufferRegion indexRegion{
                .buffer = indexBuffer,
                .offset = 0,
                .size = indexBytes};
            SDL_UploadToGPUBuffer(copyPass, &indexLocation, &indexRegion, false);
            SDL_EndGPUCopyPass(copyPass);
            if (!SDL_SubmitGPUCommandBuffer(uploadCommandBuffer)) {
                SDL_Log("Failed to submit rigid geometry upload: %s", SDL_GetError());
                SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
                SDL_ReleaseGPUBuffer(device, vertexBuffer);
                SDL_ReleaseGPUBuffer(device, indexBuffer);
                return false;
            }
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

            batches.push_back(Batch{
                batchPipeline,
                batchTexture,
                vertexBuffer,
                indexBuffer,
                static_cast<int>(vertexCount),
                COLOR_WHITE});
            return true;
        };

        const bool uploaded = wireframe
                                  ? uploadBatch(wireVertices,
                                                writesDepth ? wireframePipeline : wireframePipeline_no_depth_write,
                                                nullptr)
                                  : (texture != nullptr
                                         ? uploadBatch(texturedVertices,
                                                       usesAlpha
                                                           ? (writesDepth ? pipeline_tex_alpha
                                                                          : pipeline_tex_alpha_no_depth_write)
                                                           : (writesDepth ? rigidOpaquePipeline
                                                                          : rigidOpaquePipeline_no_depth_write),
                                                       texture)
                                         : uploadBatch(colourVertices,
                                                       usesAlpha
                                                           ? (writesDepth ? colourPipeline_alpha
                                                                          : colourPipeline_alpha_no_depth_write)
                                                           : (writesDepth ? rigidOpaqueColourPipeline
                                                                          : rigidOpaqueColourPipeline_no_depth_write),
                                                       nullptr));
        if (!uploaded) {
            break;
        }
        firstVertex += vertexCount;
    }
}
void SDLRenderer::EndRigidGeom() {}
void SDLRenderer::BeginSkinGeom(Geom* pGeom, int iSubMesh) {}
void SDLRenderer::RenderSkinInstance(render_instance& Inst) {}
void SDLRenderer::EndSkinGeom() {}
