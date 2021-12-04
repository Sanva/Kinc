#include "pipeline.h"

#include <kinc/graphics5/graphics.h>
#include <kinc/graphics5/pipeline.h>
#include <kinc/graphics5/shader.h>
#include <kinc/log.h>

#include <kinc/backend/SystemMicrosoft.h>

void kinc_g5_internal_setConstants(ID3D12GraphicsCommandList *commandList, kinc_g5_pipeline_t *pipeline) {
	/*if (currentProgram->vertexShader->constantsSize > 0) {
	    context->UpdateSubresource(currentProgram->vertexConstantBuffer, 0, nullptr, vertexConstants, 0, 0);
	    context->VSSetConstantBuffers(0, 1, &currentProgram->vertexConstantBuffer);
	}
	if (currentProgram->fragmentShader->constantsSize > 0) {
	    context->UpdateSubresource(currentProgram->fragmentConstantBuffer, 0, nullptr, fragmentConstants, 0, 0);
	    context->PSSetConstantBuffers(0, 1, &currentProgram->fragmentConstantBuffer);
	}
	if (currentProgram->geometryShader != nullptr && currentProgram->geometryShader->constantsSize > 0) {
	    context->UpdateSubresource(currentProgram->geometryConstantBuffer, 0, nullptr, geometryConstants, 0, 0);
	    context->GSSetConstantBuffers(0, 1, &currentProgram->geometryConstantBuffer);
	}
	if (currentProgram->tessControlShader != nullptr && currentProgram->tessControlShader->constantsSize > 0) {
	    context->UpdateSubresource(currentProgram->tessControlConstantBuffer, 0, nullptr, tessControlConstants, 0, 0);
	    context->HSSetConstantBuffers(0, 1, &currentProgram->tessControlConstantBuffer);
	}
	if (currentProgram->tessEvalShader != nullptr && currentProgram->tessEvalShader->constantsSize > 0) {
	    context->UpdateSubresource(currentProgram->tessEvalConstantBuffer, 0, nullptr, tessEvalConstants, 0, 0);
	    context->DSSetConstantBuffers(0, 1, &currentProgram->tessEvalConstantBuffer);
	}
	*/

	commandList->lpVtbl->SetPipelineState(commandList, pipeline->impl.pso);
#ifdef KORE_DXC
	commandList->lpVtbl->SetGraphicsRootSignature(commandList, pipeline->impl.rootSignature);
#else
	commandList->lpVtbl->SetGraphicsRootSignature(commandList, globalRootSignature);
#endif

	if (pipeline->impl.textures > 0) {
		kinc_g5_internal_set_textures(commandList);
	}
}

void kinc_g5_pipeline_init(kinc_g5_pipeline_t *pipe) {
	kinc_g5_internal_pipeline_init(pipe);
}

void kinc_g5_pipeline_destroy(kinc_g5_pipeline_t *pipe) {
	if (pipe->impl.pso != NULL) {
		pipe->impl.pso->lpVtbl->Release(pipe->impl.pso);
		pipe->impl.pso = NULL;
	}
}

// void PipelineState5Impl::set(Graphics5::PipelineState* pipeline) {
//_current = this;
// context->VSSetShader((ID3D11VertexShader*)vertexShader->shader, nullptr, 0);
// context->PSSetShader((ID3D11PixelShader*)fragmentShader->shader, nullptr, 0);

// if (geometryShader != nullptr) context->GSSetShader((ID3D11GeometryShader*)geometryShader->shader, nullptr, 0);
// if (tessControlShader != nullptr) context->HSSetShader((ID3D11HullShader*)tessControlShader->shader, nullptr, 0);
// if (tessEvalShader != nullptr) context->DSSetShader((ID3D11DomainShader*)tessEvalShader->shader, nullptr, 0);

// context->IASetInputLayout(inputLayout);
//}

#define MAX_SHADER_THING 32

static ShaderConstant findConstant(kinc_g5_shader_t *shader, const char *name) {
	if (shader != NULL) {
		for (int i = 0; i < MAX_SHADER_THING; ++i) {
			if (strcmp(shader->impl.constants[i].name, name) == 0) {
				return shader->impl.constants[i];
			}
		}
	}

	ShaderConstant constant;
	constant.name[0] = 0;
	constant.offset = -1;
	constant.size = 0;
	return constant;
}

static ShaderTexture findTexture(kinc_g5_shader_t *shader, const char *name) {
	for (int i = 0; i < MAX_SHADER_THING; ++i) {
		if (strcmp(shader->impl.textures[i].name, name) == 0) {
			return shader->impl.textures[i];
		}
	}

	ShaderTexture texture;
	texture.name[0] = 0;
	texture.texture = -1;
	return texture;
}

static ShaderAttribute findAttribute(kinc_g5_shader_t *shader, const char *name) {
	for (int i = 0; i < MAX_SHADER_THING; ++i) {
		if (strcmp(shader->impl.attributes[i].name, name) == 0) {
			return shader->impl.attributes[i];
		}
	}

	ShaderAttribute attribute;
	attribute.name[0] = 0;
	attribute.attribute = -1;
	return attribute;
}

kinc_g5_constant_location_t kinc_g5_pipeline_get_constant_location(struct kinc_g5_pipeline *pipe, const char *name) {
	kinc_g5_constant_location_t location;

	{
		ShaderConstant constant = findConstant(pipe->vertexShader, name);
		location.impl.vertexOffset = constant.offset;
		location.impl.vertexSize = constant.size;
	}

	{
		ShaderConstant constant = findConstant(pipe->fragmentShader, name);
		location.impl.fragmentOffset = constant.offset;
		location.impl.fragmentSize = constant.size;
	}

	location.impl.computeOffset = 0;
	location.impl.computeSize = 0;

	{
		ShaderConstant constant = findConstant(pipe->geometryShader, name);
		location.impl.geometryOffset = constant.offset;
		location.impl.geometrySize = constant.size;
	}

	{
		ShaderConstant constant = findConstant(pipe->tessellationControlShader, name);
		location.impl.tessControlOffset = constant.offset;
		location.impl.tessControlSize = constant.size;
	}

	{
		ShaderConstant constant = findConstant(pipe->tessellationEvaluationShader, name);
		location.impl.tessEvalOffset = constant.offset;
		location.impl.tessEvalSize = constant.size;
	}

	return location;
}

kinc_g5_texture_unit_t kinc_g5_pipeline_get_texture_unit(kinc_g5_pipeline_t *pipe, const char *name) {
	kinc_g5_texture_unit_t unit;
	ShaderTexture vertexTexture = findTexture(pipe->vertexShader, name);
	if (vertexTexture.texture != -1) {
		unit.impl.unit = vertexTexture.texture;
	}
	else {
		ShaderTexture fragmentTexture = findTexture(pipe->fragmentShader, name);
		unit.impl.unit = fragmentTexture.texture;
	}
	return unit;
}

static D3D12_BLEND convert(kinc_g5_blending_operation_t op) {
	switch (op) {
	default:
	case KINC_G5_BLEND_MODE_ONE:
		return D3D12_BLEND_ONE;
	case KINC_G5_BLEND_MODE_ZERO:
		return D3D12_BLEND_ZERO;
	case KINC_G5_BLEND_MODE_SOURCE_ALPHA:
		return D3D12_BLEND_SRC_ALPHA;
	case KINC_G5_BLEND_MODE_DEST_ALPHA:
		return D3D12_BLEND_DEST_ALPHA;
	case KINC_G5_BLEND_MODE_INV_SOURCE_ALPHA:
		return D3D12_BLEND_INV_SRC_ALPHA;
	case KINC_G5_BLEND_MODE_INV_DEST_ALPHA:
		return D3D12_BLEND_INV_DEST_ALPHA;
	case KINC_G5_BLEND_MODE_SOURCE_COLOR:
		return D3D12_BLEND_SRC_COLOR;
	case KINC_G5_BLEND_MODE_DEST_COLOR:
		return D3D12_BLEND_DEST_COLOR;
	case KINC_G5_BLEND_MODE_INV_SOURCE_COLOR:
		return D3D12_BLEND_INV_SRC_COLOR;
	case KINC_G5_BLEND_MODE_INV_DEST_COLOR:
		return D3D12_BLEND_INV_DEST_COLOR;
	}
}

static D3D12_CULL_MODE convert_cull_mode(kinc_g5_cull_mode_t cullMode) {
	switch (cullMode) {
	case KINC_G5_CULL_MODE_CLOCKWISE:
		return D3D12_CULL_MODE_FRONT;
	case KINC_G5_CULL_MODE_COUNTERCLOCKWISE:
		return D3D12_CULL_MODE_BACK;
	case KINC_G5_CULL_MODE_NEVER:
	default:
		return D3D12_CULL_MODE_NONE;
	}
}

static D3D12_COMPARISON_FUNC convert_compare_mode(kinc_g5_compare_mode_t compare) {
	switch (compare) {
	default:
	case KINC_G5_COMPARE_MODE_ALWAYS:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	case KINC_G5_COMPARE_MODE_NEVER:
		return D3D12_COMPARISON_FUNC_NEVER;
	case KINC_G5_COMPARE_MODE_EQUAL:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case KINC_G5_COMPARE_MODE_NOT_EQUAL:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case KINC_G5_COMPARE_MODE_LESS:
		return D3D12_COMPARISON_FUNC_LESS;
	case KINC_G5_COMPARE_MODE_LESS_EQUAL:
		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case KINC_G5_COMPARE_MODE_GREATER:
		return D3D12_COMPARISON_FUNC_GREATER;
	case KINC_G5_COMPARE_MODE_GREATER_EQUAL:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	}
}

static DXGI_FORMAT convert_format(kinc_g5_render_target_format_t format) {
	switch (format) {
	case KINC_G5_RENDER_TARGET_FORMAT_128BIT_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case KINC_G5_RENDER_TARGET_FORMAT_64BIT_FLOAT:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case KINC_G5_RENDER_TARGET_FORMAT_32BIT_RED_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	case KINC_G5_RENDER_TARGET_FORMAT_16BIT_RED_FLOAT:
		return DXGI_FORMAT_R16_FLOAT;
	case KINC_G5_RENDER_TARGET_FORMAT_8BIT_RED:
		return DXGI_FORMAT_R8_UNORM;
	case KINC_G5_RENDER_TARGET_FORMAT_32BIT:
	default:
#ifdef KORE_WINDOWS
		return DXGI_FORMAT_R8G8B8A8_UNORM;
#else
		return DXGI_FORMAT_B8G8R8A8_UNORM;
#endif
	}
}

static void set_blend_state(D3D12_BLEND_DESC *blend_desc, kinc_g5_pipeline_t *pipe, int target) {
	blend_desc->RenderTarget[target].BlendEnable = pipe->blendSource != KINC_G5_BLEND_MODE_ONE || pipe->blendDestination != KINC_G5_BLEND_MODE_ZERO ||
	                                               pipe->alphaBlendSource != KINC_G5_BLEND_MODE_ONE || pipe->alphaBlendDestination != KINC_G5_BLEND_MODE_ZERO;
	blend_desc->RenderTarget[target].SrcBlend = convert(pipe->blendSource);
	blend_desc->RenderTarget[target].DestBlend = convert(pipe->blendDestination);
	blend_desc->RenderTarget[target].BlendOp = D3D12_BLEND_OP_ADD;
	blend_desc->RenderTarget[target].SrcBlendAlpha = convert(pipe->alphaBlendSource);
	blend_desc->RenderTarget[target].DestBlendAlpha = convert(pipe->alphaBlendDestination);
	blend_desc->RenderTarget[target].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blend_desc->RenderTarget[target].RenderTargetWriteMask =
	    (((pipe->colorWriteMaskRed[target] ? D3D12_COLOR_WRITE_ENABLE_RED : 0) | (pipe->colorWriteMaskGreen[target] ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0)) |
	     (pipe->colorWriteMaskBlue[target] ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0)) |
	    (pipe->colorWriteMaskAlpha[target] ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0);
}

void kinc_g5_pipeline_compile(kinc_g5_pipeline_t *pipe) {
	// TODO FLOAT4x4
	int vertexAttributeCount = 0;
	for (int i = 0; i < 16; ++i) {
		if (pipe->inputLayout[i] == NULL) {
			break;
		}
		vertexAttributeCount += pipe->inputLayout[i]->size;
	}
	D3D12_INPUT_ELEMENT_DESC *vertexDesc = (D3D12_INPUT_ELEMENT_DESC *)alloca(sizeof(D3D12_INPUT_ELEMENT_DESC) * vertexAttributeCount);
	ZeroMemory(vertexDesc, sizeof(D3D12_INPUT_ELEMENT_DESC) * vertexAttributeCount);
	int curAttr = 0;
	for (int stream = 0; pipe->inputLayout[stream] != NULL; ++stream) {
		for (int i = 0; i < pipe->inputLayout[stream]->size; ++i) {
			vertexDesc[curAttr].SemanticName = "TEXCOORD";
			vertexDesc[curAttr].SemanticIndex = findAttribute(pipe->vertexShader, pipe->inputLayout[stream]->elements[i].name).attribute;
			vertexDesc[curAttr].InputSlot = stream;
			vertexDesc[curAttr].AlignedByteOffset = (i == 0) ? 0 : D3D12_APPEND_ALIGNED_ELEMENT;
			vertexDesc[curAttr].InputSlotClass =
			    pipe->inputLayout[stream]->instanced ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			vertexDesc[curAttr].InstanceDataStepRate = pipe->inputLayout[stream]->instanced ? 1 : 0;

			switch (pipe->inputLayout[stream]->elements[i].data) {
			case KINC_G4_VERTEX_DATA_F32_1:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32_FLOAT;
				break;
			case KINC_G4_VERTEX_DATA_F32_2:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32G32_FLOAT;
				break;
			case KINC_G4_VERTEX_DATA_F32_3:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32G32B32_FLOAT;
				break;
			case KINC_G4_VERTEX_DATA_F32_4:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				break;
			case KINC_G4_VERTEX_DATA_I8_1:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8_SINT;
				break;
			case KINC_G4_VERTEX_DATA_U8_1:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8_UINT;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_I8_1:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8_SNORM;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_U8_1:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8_UNORM;
				break;
			case KINC_G4_VERTEX_DATA_I8_2:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8G8_SINT;
				break;
			case KINC_G4_VERTEX_DATA_U8_2:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8G8_UINT;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_I8_2:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8G8_SNORM;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_U8_2:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8G8_UNORM;
				break;
			case KINC_G4_VERTEX_DATA_I8_4:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8G8B8A8_SINT;
				break;
			case KINC_G4_VERTEX_DATA_U8_4:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8G8B8A8_UINT;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_I8_4:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8G8B8A8_SNORM;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_U8_4:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			case KINC_G4_VERTEX_DATA_I16_1:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16_SINT;
				break;
			case KINC_G4_VERTEX_DATA_U16_1:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16_UINT;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_I16_1:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16_SNORM;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_U16_1:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16_UNORM;
				break;
			case KINC_G4_VERTEX_DATA_I16_2:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16G16_SINT;
				break;
			case KINC_G4_VERTEX_DATA_U16_2:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16G16_UINT;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_I16_2:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16G16_SNORM;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_U16_2:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16G16_UNORM;
				break;
			case KINC_G4_VERTEX_DATA_I16_4:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16G16B16A16_SINT;
				break;
			case KINC_G4_VERTEX_DATA_U16_4:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16G16B16A16_UINT;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_I16_4:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16G16B16A16_SNORM;
				break;
			case KINC_G4_VERTEX_DATA_NORMALIZED_U16_4:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R16G16B16A16_UNORM;
				break;
			case KINC_G4_VERTEX_DATA_I32_1:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32_SINT;
				break;
			case KINC_G4_VERTEX_DATA_U32_1:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32_UINT;
				break;
			case KINC_G4_VERTEX_DATA_I32_2:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32G32_SINT;
				break;
			case KINC_G4_VERTEX_DATA_U32_2:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32G32_UINT;
				break;
			case KINC_G4_VERTEX_DATA_I32_3:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32G32B32_SINT;
				break;
			case KINC_G4_VERTEX_DATA_U32_3:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32G32B32_UINT;
				break;
			case KINC_G4_VERTEX_DATA_I32_4:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32G32B32A32_SINT;
				break;
			case KINC_G4_VERTEX_DATA_U32_4:
				vertexDesc[curAttr].Format = DXGI_FORMAT_R32G32B32A32_UINT;
				break;
			}
			curAttr++;
		}
	}

	HRESULT hr;
#ifdef KORE_DXC
	hr = device->CreateRootSignature(0, pipe->vertexShader->impl.data, pipe->vertexShader->impl.length, IID_GRAPHICS_PPV_ARGS(&pipe->impl.rootSignature));
	if (hr != S_OK) {
		kinc_log(KINC_LOG_LEVEL_WARNING, "Could not create root signature.");
	}
	pipe->impl.vertexConstantsSize = pipe->vertexShader->impl.constantsSize;
	pipe->impl.fragmentConstantsSize = pipe->fragmentShader->impl.constantsSize;
#endif

	pipe->impl.textures = pipe->fragmentShader->impl.texturesCount;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {0};
	psoDesc.VS.BytecodeLength = pipe->vertexShader->impl.length;
	psoDesc.VS.pShaderBytecode = pipe->vertexShader->impl.data;
	psoDesc.PS.BytecodeLength = pipe->fragmentShader->impl.length;
	psoDesc.PS.pShaderBytecode = pipe->fragmentShader->impl.data;
#ifdef KORE_DXC
	psoDesc.pRootSignature = pipe->impl.rootSignature;
#else
	psoDesc.pRootSignature = globalRootSignature;
#endif
	psoDesc.NumRenderTargets = pipe->colorAttachmentCount;
	for (int i = 0; i < pipe->colorAttachmentCount; ++i) {
		psoDesc.RTVFormats[i] = convert_format(pipe->colorAttachment[i]);
	}
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	psoDesc.InputLayout.NumElements = vertexAttributeCount;
	psoDesc.InputLayout.pInputElementDescs = vertexDesc;

	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = convert_cull_mode(pipe->cullMode);
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
	    FALSE,
	    FALSE,
	    D3D12_BLEND_ONE,
	    D3D12_BLEND_ZERO,
	    D3D12_BLEND_OP_ADD,
	    D3D12_BLEND_ONE,
	    D3D12_BLEND_ZERO,
	    D3D12_BLEND_OP_ADD,
	    D3D12_LOGIC_OP_NOOP,
	    D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
		psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
	}

	bool independentBlend = false;
	for (int i = 1; i < 8; ++i) {
		if (pipe->colorWriteMaskRed[0] != pipe->colorWriteMaskRed[i] || pipe->colorWriteMaskGreen[0] != pipe->colorWriteMaskGreen[i] ||
		    pipe->colorWriteMaskBlue[0] != pipe->colorWriteMaskBlue[i] || pipe->colorWriteMaskAlpha[0] != pipe->colorWriteMaskAlpha[i]) {
			independentBlend = true;
			break;
		}
	}

	set_blend_state(&psoDesc.BlendState, pipe, 0);
	if (independentBlend) {
		psoDesc.BlendState.IndependentBlendEnable = true;
		for (int i = 1; i < 8; ++i) {
			set_blend_state(&psoDesc.BlendState, pipe, i);
		}
	}

	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
	psoDesc.DepthStencilState.FrontFace = defaultStencilOp;
	psoDesc.DepthStencilState.BackFace = defaultStencilOp;

	psoDesc.DepthStencilState.DepthEnable = pipe->depthMode != KINC_G5_COMPARE_MODE_ALWAYS;
	psoDesc.DepthStencilState.DepthWriteMask = pipe->depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = convert_compare_mode(pipe->depthMode);
	psoDesc.DepthStencilState.StencilEnable = false;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = 0xFFFFFFFF;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	hr = device->lpVtbl->CreateGraphicsPipelineState(device, &psoDesc, &IID_ID3D12PipelineState, &pipe->impl.pso);
	if (hr != S_OK) {
		kinc_log(KINC_LOG_LEVEL_WARNING, "Could not create pipeline.");
	}
}

void kinc_g5_compute_pipeline_init(kinc_g5_compute_pipeline_t *pipeline) {
	kinc_g5_internal_compute_pipeline_init(pipeline);
	pipeline->impl.pso = NULL;
}

void kinc_g5_compute_pipeline_destroy(kinc_g5_compute_pipeline_t *pipeline) {
	if (pipeline->impl.pso != NULL) {
		pipeline->impl.pso->lpVtbl->Release(pipeline->impl.pso);
		pipeline->impl.pso = NULL;
	}
}

void kinc_g5_compute_pipeline_compile(kinc_g5_compute_pipeline_t *pipeline) {
	HRESULT hr;
#ifdef KORE_DXC
	/*hr = device->CreateRootSignature(0, pipe->vertexShader->impl.data, pipe->vertexShader->impl.length, IID_GRAPHICS_PPV_ARGS(&pipe->impl.rootSignature));
	if (hr != S_OK) {
	    kinc_log(KINC_LOG_LEVEL_WARNING, "Could not create root signature.");
	}
	pipe->impl.vertexConstantsSize = pipe->vertexShader->impl.constantsSize;
	pipe->impl.fragmentConstantsSize = pipe->fragmentShader->impl.constantsSize;*/
#endif

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {0};
	psoDesc.CS.BytecodeLength = pipeline->compute_shader->impl.length;
	psoDesc.CS.pShaderBytecode = pipeline->compute_shader->impl.data;
#ifdef KORE_DXC
	// psoDesc.pRootSignature = pipe->impl.rootSignature;
#else
	psoDesc.pRootSignature = globalComputeRootSignature;
#endif

	hr = device->lpVtbl->CreateComputePipelineState(device, &psoDesc, &IID_ID3D12PipelineState, &pipeline->impl.pso);
	if (hr != S_OK) {
		kinc_log(KINC_LOG_LEVEL_WARNING, "Could not create pipeline.");
	}
}

kinc_g5_constant_location_t kinc_g5_compute_pipeline_get_constant_location(kinc_g5_compute_pipeline_t *pipeline, const char *name) {
	kinc_g5_constant_location_t location = {0};

	{
		ShaderConstant constant = findConstant(pipeline->compute_shader, name);
		location.impl.computeOffset = constant.offset;
		location.impl.computeSize = constant.size;
	}

	return location;
}

kinc_g5_texture_unit_t kinc_g5_compute_pipeline_get_texture_unit(kinc_g5_compute_pipeline_t *pipeline, const char *name) {
	kinc_g5_texture_unit_t unit;
	ShaderTexture vertexTexture = findTexture(pipeline->compute_shader, name);
	if (vertexTexture.texture != -1) {
		unit.impl.unit = vertexTexture.texture;
	}
	else {
		ShaderTexture fragmentTexture = findTexture(pipeline->compute_shader, name);
		unit.impl.unit = fragmentTexture.texture;
	}
	return unit;
}
