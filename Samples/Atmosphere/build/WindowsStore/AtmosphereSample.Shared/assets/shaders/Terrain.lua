-- Terrain.lua

-- Externally defined variables:
-- extResourceMapping



LinearMirrorSampler = Sampler.Create{
    Name = "Terrain.lua: linear mirror sampler",
    MinFilter = "FILTER_TYPE_LINEAR", 
    MagFilter = "FILTER_TYPE_LINEAR", 
    MipFilter = "FILTER_TYPE_LINEAR", 
    AddressU = "TEXTURE_ADDRESS_MIRROR", 
    AddressV = "TEXTURE_ADDRESS_MIRROR", 
    AddressW = "TEXTURE_ADDRESS_MIRROR"
}

PointClampSampler = Sampler.Create{
    Name = "Terrain.lua: point clamp sampler",
    MinFilter = "FILTER_TYPE_POINT", 
    MagFilter = "FILTER_TYPE_POINT", 
    MipFilter = "FILTER_TYPE_POINT", 
    AddressU = "TEXTURE_ADDRESS_CLAMP", 
    AddressV = "TEXTURE_ADDRESS_CLAMP", 
    AddressW = "TEXTURE_ADDRESS_CLAMP"
}

LinearClampSampler = Sampler.Create{
    Name = "Terrain.lua: linear clamp sampler",
    MinFilter = "FILTER_TYPE_LINEAR", 
    MagFilter = "FILTER_TYPE_LINEAR", 
    MipFilter = "FILTER_TYPE_LINEAR", 
    AddressU = "TEXTURE_ADDRESS_CLAMP", 
    AddressV = "TEXTURE_ADDRESS_CLAMP", 
    AddressW = "TEXTURE_ADDRESS_CLAMP"
}

LinearWrapSampler = Sampler.Create{
    Name = "Terrain.lua: linear wrap sampler",
    MinFilter = "FILTER_TYPE_LINEAR", 
    MagFilter = "FILTER_TYPE_LINEAR", 
    MipFilter = "FILTER_TYPE_LINEAR", 
    AddressU = "TEXTURE_ADDRESS_WRAP", 
    AddressV = "TEXTURE_ADDRESS_WRAP", 
    AddressW = "TEXTURE_ADDRESS_WRAP"
}

ComparisonSampler = Sampler.Create{
    Name = "Terrain.lua: comparison sampler",
    MinFilter = "FILTER_TYPE_COMPARISON_LINEAR", 
    MagFilter = "FILTER_TYPE_COMPARISON_LINEAR", 
    MipFilter = "FILTER_TYPE_COMPARISON_LINEAR", 
    AddressU = "TEXTURE_ADDRESS_CLAMP", 
    AddressV = "TEXTURE_ADDRESS_CLAMP", 
    AddressW = "TEXTURE_ADDRESS_CLAMP",
	ComparisonFunc = "COMPARISON_FUNC_LESS",
}

extResourceMapping["g_tex2DNormalMap"]:SetSampler(LinearMirrorSampler)
extResourceMapping["g_tex2DMtrlMap"]:SetSampler(LinearMirrorSampler)
extResourceMapping["g_tex2DTileDiffuse0"]:SetSampler(LinearWrapSampler)
extResourceMapping["g_tex2DTileDiffuse1"]:SetSampler(LinearWrapSampler)
extResourceMapping["g_tex2DTileDiffuse2"]:SetSampler(LinearWrapSampler)
extResourceMapping["g_tex2DTileDiffuse3"]:SetSampler(LinearWrapSampler)
extResourceMapping["g_tex2DTileDiffuse4"]:SetSampler(LinearWrapSampler)
extResourceMapping["g_tex2DTileNM0"]:SetSampler(LinearWrapSampler)
extResourceMapping["g_tex2DTileNM1"]:SetSampler(LinearWrapSampler)
extResourceMapping["g_tex2DTileNM2"]:SetSampler(LinearWrapSampler)
extResourceMapping["g_tex2DTileNM3"]:SetSampler(LinearWrapSampler)
extResourceMapping["g_tex2DTileNM4"]:SetSampler(LinearWrapSampler)

-- Shaders

function CreateHemisphereShaders()

	HemisphereVS = Shader.Create{
		FilePath =  "HemisphereVS.fx",
		EntryPoint = "HemisphereVS",
		SearchDirectories = "shaders;shaders\\terrain",
		SourceLanguage = "SHADER_SOURCE_LANGUAGE_HLSL",
		Desc = {
			ShaderType = "SHADER_TYPE_VERTEX",
			Name = "HemisphereVS",
			DefaultVariableType = "SHADER_VARIABLE_TYPE_STATIC",
			VariableDesc = 
			{ 
				{Name = "g_CameraAttribs", Type = "SHADER_VARIABLE_TYPE_MUTABLE"}, 
				{Name = "g_LightAttribs", Type = "SHADER_VARIABLE_TYPE_MUTABLE"}, 
				{Name = "g_TerrainAttribs", Type = "SHADER_VARIABLE_TYPE_MUTABLE"},
				{Name = "g_MediaParams", Type = "SHADER_VARIABLE_TYPE_STATIC"},
				{Name = "g_tex2DOccludedNetDensityToAtmTop", Type = "SHADER_VARIABLE_TYPE_DYNAMIC"},
				{Name = "g_tex2DAmbientSkylight", Type = "SHADER_VARIABLE_TYPE_DYNAMIC"} 
			}
		}
	}

	assert(HemisphereVS.Desc.VariableDesc[1].Name == "g_CameraAttribs")
	assert(HemisphereVS.Desc.VariableDesc[1].Type == "SHADER_VARIABLE_TYPE_MUTABLE")

	assert(HemisphereVS.Desc.VariableDesc[4].Name == "g_MediaParams")
	assert(HemisphereVS.Desc.VariableDesc[4].Type == "SHADER_VARIABLE_TYPE_STATIC")

	assert(HemisphereVS.Desc.VariableDesc[6].Name == "g_tex2DAmbientSkylight")
	assert(HemisphereVS.Desc.VariableDesc[6].Type == "SHADER_VARIABLE_TYPE_DYNAMIC")

	HemisphereZOnlyVS = Shader.Create{
		FilePath =  "HemisphereZOnlyVS.fx",
		EntryPoint = "HemisphereZOnlyVS",
		SourceLanguage = "SHADER_SOURCE_LANGUAGE_HLSL",
		SearchDirectories = "shaders\\;shaders\\terrain",
		Desc = {
			ShaderType = "SHADER_TYPE_VERTEX",
			Name = "HemisphereZOnlyVS",
			DefaultVariableType = "SHADER_VARIABLE_TYPE_STATIC",
			VariableDesc = 
			{ 
				{Name = "g_CameraAttribs", Type = "SHADER_VARIABLE_TYPE_STATIC"}, 
			}
		}
	}

	-- extResourceMapping is set by the app
	HemisphereVS:BindResources(extResourceMapping)
	HemisphereZOnlyVS:BindResources(extResourceMapping)
end

function SetHemispherePS(in_HemispherePS, RTVFormat)
	HemispherePS = in_HemispherePS
	HemispherePS:BindResources(extResourceMapping)
	
	InputLayoutElements = 
	{
			{ InputIndex = 0, BufferSlot = 0, NumComponents = 3, ValueType = "VT_FLOAT32"},
			{ InputIndex = 1, BufferSlot = 0, NumComponents = 2, ValueType = "VT_FLOAT32"}
	}

	RenderHemispherePSO = PipelineState.Create
	{
		Name = "RenderHemisphere",
		GraphicsPipeline = 
		{
			DepthStencilDesc = 
			{
				DepthEnable = true,
				DepthWriteEnable = true,
				DepthFunc = "COMPARISON_FUNC_LESS"
			},
			RasterizerDesc = 
			{
				FillMode = "FILL_MODE_SOLID",
				--FillMode = "FILL_MODE_WIREFRAME",
				CullMode = "CULL_MODE_BACK",
				FrontCounterClockwise = true
			},
			InputLayout = InputLayoutElements,
			pVS = HemisphereVS,
			pPS = HemispherePS,
			RTVFormats = RTVFormat,
			DSVFormat = "TEX_FORMAT_D32_FLOAT"
		}
	}
	RenderHemisphereSRB = RenderHemispherePSO:CreateShaderResourceBinding()
	RenderHemisphereSRB:BindResources("SHADER_TYPE_VERTEX", extResourceMapping, "BIND_SHADER_RESOURCES_UPDATE_UNRESOLVED")

	RenderHemisphereZOnlyPSO = PipelineState.Create
	{
		Name = "Render Hemisphere Z Only",
		GraphicsPipeline = 
		{
			DepthStencilDesc = 
			{
				DepthEnable = true,
				DepthWriteEnable = true,
				DepthFunc = "COMPARISON_FUNC_LESS"
			},
			RasterizerDesc = 
			{
				FillMode = "FILL_MODE_SOLID",
				CullMode = "CULL_MODE_BACK",
				DepthClipEnable = false,
				FrontCounterClockwise = false
				-- Do not use slope-scaled depth bias because this results in light leaking
				-- through terrain!			
			},
			InputLayout = InputLayoutElements,
			pVS = HemisphereZOnlyVS,
			DSVFormat = "TEX_FORMAT_D32_FLOAT"
		}
	}
end


function CreateRenderNormalMapShaders()

	local ScreenSizeQuadVS = Shader.Create{
		FilePath =  "ScreenSizeQuadVS.fx",
		EntryPoint = "GenerateScreenSizeQuadVS",
		SearchDirectories = "shaders\\;shaders\\terrain",
		SourceLanguage = "SHADER_SOURCE_LANGUAGE_HLSL",
		Desc = {
			ShaderType = "SHADER_TYPE_VERTEX",
			Name = "GenerateScreenSizeQuadVS",
			DefaultVariableType = "SHADER_VARIABLE_TYPE_STATIC"
		}
	}

	local GenerateNormalMapPS = Shader.Create{
		FilePath =  "GenerateNormalMapPS.fx",
		EntryPoint = "GenerateNormalMapPS",
		SearchDirectories = "shaders\\;shaders\\terrain",
		SourceLanguage = "SHADER_SOURCE_LANGUAGE_HLSL",
		Desc = {
			ShaderType = "SHADER_TYPE_PIXEL",
			Name = "GenerateNormalMapPS",
			DefaultVariableType = "SHADER_VARIABLE_TYPE_STATIC",
			VariableDesc = 
			{ 
				{Name = "g_tex2DElevationMap", Type = "SHADER_VARIABLE_TYPE_STATIC"}, 
				{Name = "g_NMGenerationAttribs", Type = "SHADER_VARIABLE_TYPE_STATIC"}, 
			}
		}
	}

	extResourceMapping["g_tex2DElevationMap"]:SetSampler(PointClampSampler)
	ScreenSizeQuadVS:BindResources(extResourceMapping, "BIND_SHADER_RESOURCES_RESET_BINDINGS")
	GenerateNormalMapPS:BindResources(extResourceMapping, "BIND_SHADER_RESOURCES_RESET_BINDINGS")


	RenderNormalMapPSO = PipelineState.Create
	{
		GraphicsPipeline = 
		{
			DepthStencilDesc = 
			{
				DepthEnable = false,
				DepthWriteEnable = false
			},
			RasterizerDesc = 
			{
				FillMode = "FILL_MODE_SOLID",
				CullMode = "CULL_MODE_NONE",
				FrontCounterClockwise = true
			},
			pVS = ScreenSizeQuadVS,
			pPS = GenerateNormalMapPS,
			RTVFormats = "TEX_FORMAT_RG8_UNORM"
		}
	}
	
end


function SetRenderNormalMapShadersAndStates()
	Context.SetPipelineState(RenderNormalMapPSO)
	Context.CommitShaderResources()
end

function RenderHemisphere(PrecomputedNetDensitySRV, AmbientSkylightSRV)
	Context.SetPipelineState(RenderHemispherePSO)
	
	RenderHemisphereSRB:GetVariable("SHADER_TYPE_VERTEX", "g_tex2DOccludedNetDensityToAtmTop"):Set(PrecomputedNetDensitySRV)
	RenderHemisphereSRB:GetVariable("SHADER_TYPE_VERTEX", "g_tex2DAmbientSkylight"):Set(AmbientSkylightSRV)

	Context.CommitShaderResources(RenderHemisphereSRB)
end

function RenderHemisphereShadow()
	Context.SetPipelineState(RenderHemisphereZOnlyPSO)
	Context.CommitShaderResources()
end
