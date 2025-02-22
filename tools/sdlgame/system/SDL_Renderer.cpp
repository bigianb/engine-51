#include "SDL_Renderer.h"

#include <SDL3/SDL.h>

#include"../../../a51lib/Bitmap.h"

SDLRenderer::SDLRenderer()
{

}

SDL_GPUShader* LoadShader(
	SDL_GPUDevice* device,
    const char* basePath,
	const char* shaderFilename,
	Uint32 samplerCount,
	Uint32 uniformBufferCount,
	Uint32 storageBufferCount,
	Uint32 storageTextureCount
) {
	// Auto-detect the shader stage from the file name for convenience
	SDL_GPUShaderStage stage;
	if (SDL_strstr(shaderFilename, ".vert"))
	{
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(shaderFilename, ".frag"))
	{
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else
	{
		SDL_Log("Invalid shader stage!");
		return nullptr;
	}

	char fullPath[256];
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entrypoint;

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/compiled/SPIRV/%s.spv", basePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/compiled/MSL/%s.msl", basePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/compiled/DXIL/%s.dxil", basePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} else {
		SDL_Log("%s", "Unrecognized backend shader format!");
		return nullptr;
	}

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	if (code == nullptr)
	{
		SDL_Log("Failed to load shader from disk! %s", fullPath);
		return nullptr;
	}

	SDL_GPUShaderCreateInfo shaderInfo = {
        .code_size = codeSize,
		.code = (const Uint8 *)code,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = samplerCount,
        .num_storage_textures = storageTextureCount,
		.num_storage_buffers = storageBufferCount,
        .num_uniform_buffers = uniformBufferCount
	};
	SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
	if (shader == nullptr)
	{
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
	if (vertexShader == nullptr)
	{
		SDL_Log("Failed to create vertex shader!");
		return false;
	}

	SDL_GPUShader* fragmentShader = LoadShader(device, basePath, "Textured.frag", 1, 0, 0, 0);
	if (fragmentShader == nullptr)
	{
		SDL_Log("Failed to create fragment shader!");
		return false;
	}

    return true;
}

void SDLRenderer::quit()
{
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyWindow(window);
    SDL_DestroyGPUDevice(device);
}

void SDLRenderer::drawBegin(Primitive, int drawFlags)
{
}

void SDLRenderer::drawEnd()
{
}

void SDLRenderer::setTexture(Bitmap* tex)
{

}

void SDLRenderer::drawUV(float u, float v)
{
}

void SDLRenderer::drawColour(const Colour& colour)
{
}

void SDLRenderer::drawVertex(float x, float y, float z)
{
}
