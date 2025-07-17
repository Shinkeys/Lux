#pragma once
#include "../../util/gfx/vk_types.h"


#include <slang-com-ptr.h>
#include <slang.h>



class VulkanDevice;
class VulkanShader
{
private:
	VulkanDevice& _deviceObj;

	std::unordered_map<std::string, slang::IModule*> _modulesStorage;

	Slang::ComPtr<slang::IGlobalSession> _globalSession;
	Slang::ComPtr<slang::ISession> _localSession;
public:
	VulkanShader(VulkanDevice& deviceObj);
	~VulkanShader() = default;

	VulkanShader(const VulkanShader&) = delete;
	VulkanShader(VulkanShader&&) = delete;
	VulkanShader& operator= (const VulkanShader&) = delete;
	VulkanShader& operator= (VulkanShader&&) = delete;

	slang::IModule* LoadModule(const fs::path& shaderPath);

	slang::IModule* GetModuleByName(const std::string& name);

	VkShaderModule CreateShaderModule(slang::IModule* slangModule, const std::string& entrypoint);
};