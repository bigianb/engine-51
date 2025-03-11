#include "SDL_Renderer.h"

#include <SDL3/SDL.h>
#include <cassert>

#include "../../../a51lib/Bitmap.h"

SDLRenderer::SDLRenderer()
{
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

    SDL_GPUShader* fragmentShader = LoadShader(device, basePath, "Textured.frag", 1, 1, 0, 0);
    if (fragmentShader == nullptr) {
        SDL_Log("Failed to create fragment shader!");
        return false;
    }

    SDL_GPUShader* positionColorVertexShader = LoadShader(device, basePath, "PositionColor.vert", 0, 0, 0, 0);
    if (vertexShader == nullptr) {
        SDL_Log("Failed to create vertex shader!");
        return false;
    }

    SDL_GPUShader* solidColorFragmentShader = LoadShader(device, basePath, "SolidColor.frag", 1, 1, 0, 0);
    if (fragmentShader == nullptr) {
        SDL_Log("Failed to create fragment shader!");
        return false;
    }

    // Probably need multiple of these.
    SDL_GPUColorTargetDescription ctd[]{
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

    SDL_GPUVertexAttribute vba[]{
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 0,
        },
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = sizeof(float) * 3,
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
        .num_vertex_attributes = 2,
    };

    SDL_GPUVertexAttribute positionColourVba[]{
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 0,
        },
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = sizeof(float) * 3,
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
        .compare_op = SDL_GPU_COMPAREOP_INVALID,
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
        .enable_depth_test = false,
        .enable_depth_write = false,
        .enable_stencil_test = false,
    };

    {
        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .vertex_shader = vertexShader,
            .fragment_shader = fragmentShader,
            .vertex_input_state = vertexInputState,
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .rasterizer_state = rasteriserState,
            .multisample_state = multisampleState,
            .depth_stencil_state = depthStencilState,
            .target_info = {
                .color_target_descriptions = ctd,
                .num_color_targets = 1,

            },
            .props = 0,
        };

        pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);
        if (pipeline == nullptr) {
            SDL_Log("Failed to create pipeline!");
            return false;
        }
    }
    {
        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .vertex_shader = positionColorVertexShader,
            .fragment_shader = solidColorFragmentShader,
            .vertex_input_state = positionColourVertexInputState,
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .rasterizer_state = rasteriserState,
            .multisample_state = multisampleState,
            .depth_stencil_state = depthStencilState,
            .target_info = {
                .color_target_descriptions = ctd,
                .num_color_targets = 1,

            },
            .props = 0,
        };

        colourPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);
        if (colourPipeline == nullptr) {
            SDL_Log("Failed to create colourPipeline!");
            return false;
        }
    }
    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);
    SDL_ReleaseGPUShader(device, positionColorVertexShader);
    SDL_ReleaseGPUShader(device, solidColorFragmentShader);

    SDL_GPUSamplerCreateInfo sci{
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    pointSampler = SDL_CreateGPUSampler(device, &sci);

    return true;
}

void SDLRenderer::quit()
{
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    pipeline = nullptr;

    SDL_ReleaseGPUGraphicsPipeline(device, colourPipeline);
    colourPipeline = nullptr;

    for (const auto& [key, value] : gpuTextures) {
        SDL_ReleaseGPUTexture(device, value);
    }

    gpuTextures.clear();

    SDL_ReleaseGPUSampler(device, pointSampler);

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyWindow(window);
    SDL_DestroyGPUDevice(device);
}

struct FragMultiplyUniform
{
    float r, g, b, a;
};

void SDLRenderer::draw()
{
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(device);
    if (cmdbuf == nullptr) {
        SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, window, &swapchainTexture, nullptr, nullptr)) {
        SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return;
    }

    if (swapchainTexture != nullptr) {
        SDL_FColor             black{0.0f, 0.0f, 0.0f, 1.0f};
        SDL_GPUColorTargetInfo colorTargetInfo = {0};
        colorTargetInfo.texture = swapchainTexture;
        colorTargetInfo.clear_color = black;
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);
        
        for (auto& batch : batches) {
            SDL_BindGPUGraphicsPipeline(renderPass, batch.pipeline);
            SDL_GPUBufferBinding bb{.buffer = batch.vertexBuffer, .offset = 0};
            SDL_BindGPUVertexBuffers(renderPass, 0, &bb, 1);
            SDL_GPUBufferBinding bbi{.buffer = batch.indexBuffer, .offset = 0};
            SDL_BindGPUIndexBuffer(renderPass, &bbi, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            if (batch.texture != nullptr){
                SDL_GPUTextureSamplerBinding tsb{.texture = batch.texture, .sampler = pointSampler};
                SDL_BindGPUFragmentSamplers(renderPass, 0, &tsb, 1);

                FragMultiplyUniform uni{batch.colour.r / 255.0f, batch.colour.g / 255.0f, batch.colour.b / 255.0f, batch.colour.a / 255.0f};

                SDL_PushGPUFragmentUniformData(cmdbuf, 0, &uni, sizeof(FragMultiplyUniform));
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
    float wx = (x - 320.0f) / 320.0f;
    float wy = (240.0f - y) / 240.0f;
    accumulatedVertices.push_back({wx, wy, z, u, v});
}

void SDLRenderer::drawColourVertex(float x, float y, float z, const Colour& c)
{
    float wx = (x - 320.0f) / 320.0f;
    float wy = (240.0f - y) / 240.0f;
    accumulatedColourVertices.push_back({wx, wy, z, c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f});
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

    // TODO: delay upload until render time?
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

    batches.push_back(Batch{pipeline, gpuTextures[currentTex], vertexBuffer, indexBuffer, (int)accumulatedVertices.size(), currentColour});

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
    SDL_GPUTransferBufferCreateInfo tbci{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = (Uint32)(tex->width * tex->height * 4),
    };
    SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(
        device,
        &tbci);

    Uint8* textureTransferPtr = (Uint8*)SDL_MapGPUTransferBuffer(
        device,
        textureTransferBuffer,
        false);
    SDL_memcpy(textureTransferPtr, (const uint8_t*)tex->getPixelData(0), tex->width * tex->height * 4);
    SDL_UnmapGPUTransferBuffer(device, textureTransferBuffer);

    // Upload the transfer data to the GPU resources
    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass*      copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

    SDL_GPUTextureTransferInfo tti{
        .transfer_buffer = textureTransferBuffer,
        .offset = 0, /* Zeros out the rest */
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
