#include "../../../headers/base/gfx/vk_shader.h"
#include "../../../headers/base/gfx/vk_device.h"
#include "../../../headers/util/gfx/vk_helpers.h"


using Slang::ComPtr;


#define SLANG_CHECK(x) \
  {                       \
    auto _res = x;        \
    if (_res != 0)        \
    {                     \
        assert(false);    \
    }                     \
  }




void PrintDiagnosticBlob(ComPtr<slang::IBlob> blob)
{
#ifndef NDEBUG
	if (blob != nullptr)
	{
		printf("%s", (const char*)blob->getBufferPointer());
	}
#endif
}



VulkanShader::VulkanShader(VulkanDevice& deviceObj) : _deviceObj{deviceObj}
{
	SLANG_CHECK(slang::createGlobalSession(_globalSession.writeRef()));

    slang::SessionDesc sessionDesc{};
    slang::TargetDesc  targetDesc{};


    targetDesc.format  = SLANG_SPIRV;
    targetDesc.profile = _globalSession->findProfile("sm_6_8");
	targetDesc.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
	targetDesc.forceGLSLScalarBufferLayout = true;


    sessionDesc.targets = &targetDesc;	
    sessionDesc.targetCount = 1;
    sessionDesc.compilerOptionEntryCount = 0;
	sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR; // glsl like

    SLANG_CHECK(_globalSession->createSession(sessionDesc, _localSession.writeRef()));
}

slang::IModule* VulkanShader::GetModuleByName(const std::string& name)
{
	auto it = _modulesStorage.find(name);
	if (it == _modulesStorage.end())
		return nullptr;

	return it->second;
}

VkShaderModule VulkanShader::CreateShaderModule(slang::IModule* slangModule, const std::string& entrypointName)
{
	assert(slangModule && "Can't create shader module, slang module is nullptr");


	ComPtr<slang::IEntryPoint> entryPoint;
	SlangResult result = slangModule->findEntryPointByName(entrypointName.c_str(), entryPoint.writeRef());
	if (result != 0)
	{
		std::cout << "Unable to create shader module by entrypoint: " << entrypointName << '\n';
		assert(false);
	}


	ComPtr<slang::IComponentType> linkedProgram;
	{
		ComPtr<slang::IBlob> diagnosticsBlob;
		SlangResult result = entryPoint->link(linkedProgram.writeRef(), diagnosticsBlob.writeRef());
		PrintDiagnosticBlob(diagnosticsBlob);

		SLANG_CHECK(result);
	}

	ComPtr<slang::IBlob> spirv;
	{
		ComPtr<slang::IBlob> diagnosticsBlob;
		SlangResult result = linkedProgram->getEntryPointCode(
			0,
			0,
			spirv.writeRef(),
			diagnosticsBlob.writeRef());
		PrintDiagnosticBlob(diagnosticsBlob);

		SLANG_CHECK(result);
	}
	 

	VkShaderModule shaderModule = vkhelpers::ReadShaderFile(static_cast<const u32*>(spirv->getBufferPointer()),
		spirv->getBufferSize(), _deviceObj.GetDevice());


	return shaderModule;
}

slang::IModule* VulkanShader::LoadModule(const fs::path& shaderPath)
{
	fs::path currentDir = fs::current_path();
	while (!helpers::IsProjectRoot(currentDir))
	{
		currentDir = currentDir.parent_path();
	}

	fs::path changedShaderPath = currentDir / "resources" / "shaders" / shaderPath;

	slang::IModule* slangModule = nullptr;
  	if (_modulesStorage.find(shaderPath.string()) == _modulesStorage.end())
	{
		ComPtr<slang::IBlob> diagnosticsBlob;
		slangModule = _localSession->loadModule(changedShaderPath.string().c_str(), diagnosticsBlob.writeRef());
		PrintDiagnosticBlob(diagnosticsBlob);
		if (!slangModule)
			assert(false);
	}
	else
		slangModule = _modulesStorage[changedShaderPath.string()];

	return slangModule;
}