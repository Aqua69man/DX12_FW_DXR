#include "DxrGame.h"

#include "External/DXCAPI/dxcapi.use.h"
#include <d3dcompiler.h>
#include <fstream>	// ifstream
#include <sstream>  // stringstream

static dxc::DxcDllSupport gDxcDllHelper;

// =====================================================================================
//										Global vars 
// =====================================================================================
using namespace Microsoft::WRL;
using namespace DirectX;

// Clamp a value between a min and max range.
template<typename T>
constexpr const T& clamp(const T& val, const T& min, const T& max)
{
	return val < min ? min : val > max ? max : val;
}

// Vertex data.
struct VertexPos
{
	XMFLOAT3 Position;
};

// Vertex data for a colored primitives.
struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static WORD g_Indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

// Convert a blob to string
template<class BlobType>
std::string convertBlobToString(BlobType* pBlob)
{
	std::vector<char> infoLog(pBlob->GetBufferSize() + 1);
	memcpy(infoLog.data(), pBlob->GetBufferPointer(), pBlob->GetBufferSize());
	infoLog[pBlob->GetBufferSize()] = 0;
	return std::string(infoLog.data());
}

#define arraysize(a) (sizeof(a)/sizeof(a[0]))

// =====================================================================================
//						  Build ACCELERATION_STRUCTURE Helper Funcs
// =====================================================================================

static const D3D12_HEAP_PROPERTIES kUploadHeapProps =
{
	D3D12_HEAP_TYPE_UPLOAD,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0,
};

static const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
{
	D3D12_HEAP_TYPE_DEFAULT,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0
};

ComPtr<ID3D12Resource> DxrGame::CreateBuffer(ComPtr<ID3D12Device5> pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, 
	D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps)
{
	D3D12_RESOURCE_DESC bufDesc = {};
	bufDesc.Alignment = 0;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Flags = flags;
	bufDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufDesc.Height = 1;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufDesc.MipLevels = 1;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.SampleDesc.Quality = 0;
	bufDesc.Width = size;

	ComPtr<ID3D12Resource> pBuffer;
	ThrowIfFailed(pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer)));
	return pBuffer;
}

ComPtr<ID3D12Resource> DxrGame::CreateTriangleVB(ComPtr<ID3D12Device5> pDevice)
{
	const VertexPos vertices[] =
	{
		XMFLOAT3(0,          1,  0),
		XMFLOAT3(0.866f,  -0.5f, 0),
		XMFLOAT3(-0.866f, -0.5f, 0),
	};

	// For simplicity, we create the vertex buffer on the upload heap, but that's not required
	ComPtr<ID3D12Resource> pBuffer = CreateBuffer(pDevice, sizeof(vertices), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	uint8_t* pData;
	pBuffer->Map(0, nullptr, (void**)&pData);
	memcpy(pData, vertices, sizeof(vertices));
	pBuffer->Unmap(0, nullptr);
	return pBuffer;
}

ComPtr<ID3D12Resource> DxrGame::CreatePlaneVB(ComPtr<ID3D12Device5> pDevice)
{
	const VertexPos vertices[] =
	{
		XMFLOAT3(-100, -1,  -2),
		XMFLOAT3(100, -1,  100),
		XMFLOAT3(-100, -1,  100),

		XMFLOAT3(-100, -1,  -2),
		XMFLOAT3(100, -1,  -2),
		XMFLOAT3(100, -1,  100),
	};

	// For simplicity, we create the vertex buffer on the upload heap, but that's not required
	ComPtr<ID3D12Resource> pBuffer = CreateBuffer(pDevice, sizeof(vertices), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	uint8_t* pData;
	pBuffer->Map(0, nullptr, (void**)&pData);
	memcpy(pData, vertices, sizeof(vertices));
	pBuffer->Unmap(0, nullptr);
	return pBuffer;
}

DxrGame::AccelerationStructureBuffers DxrGame::CreateBottomLevelAS(ComPtr<ID3D12Device5> pDevice, ComPtr<ID3D12GraphicsCommandList4> pCmdList,
	ComPtr<ID3D12Resource> pVB[], const uint32_t vertexCount[], uint32_t geometryCount)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDesc;
	geomDesc.resize(geometryCount);

	for (uint32_t i = 0; i < geometryCount; i++)
	{
		geomDesc[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc[i].Triangles.VertexBuffer.StartAddress = pVB[i]->GetGPUVirtualAddress();
		geomDesc[i].Triangles.VertexBuffer.StrideInBytes = sizeof(XMFLOAT3);
		geomDesc[i].Triangles.VertexCount = vertexCount[i];
		geomDesc[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geomDesc[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}

	// Get the size requirements for the scratch and AS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = geometryCount;
	inputs.pGeometryDescs = geomDesc.data();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
	DxrGame::AccelerationStructureBuffers buffers;
	buffers.pScratch = CreateBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, kDefaultHeapProps);
	buffers.pResult = CreateBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);

	// Create the bottom-level AS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

	pCmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = buffers.pResult.Get();
	pCmdList->ResourceBarrier(1, &uavBarrier);

	return buffers;
}

void DxrGame::BuildTopLevelAS(ComPtr<ID3D12Device5> pDevice, ComPtr<ID3D12GraphicsCommandList4> pCmdList, ComPtr<ID3D12Resource> pBottomLevelAS[2], 
	uint64_t& tlasSize, float rotation, bool update, DxrGame::AccelerationStructureBuffers& buffers)
{
	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;		// <---- if (update)
	inputs.NumDescs = 3;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (update)
	{
		// If this a request for an update, then the TLAS was already used in a DispatchRay() call. We need a UAV barrier to make sure the read operation ends before updating the buffer
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = buffers.pResult.Get();
		pCmdList->ResourceBarrier(1, &uavBarrier);
	}
	else
	{
		// If this is not an update operation then we need to create the buffers, otherwise we will refit in-place
		buffers.pScratch = CreateBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
		buffers.pResult = CreateBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
		buffers.pInstanceDesc = CreateBuffer(pDevice, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		tlasSize = info.ResultDataMaxSizeInBytes;
	}

	// Map the instance desc buffer
	{
		// Map
		D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
		buffers.pInstanceDesc->Map(0, nullptr, (void**)&instanceDescs);
		{
			ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3);

			const XMVECTOR rotationAxisY = XMVectorSet(0, 1, 0, 0);
			DirectX::XMMATRIX rotationMat = DirectX::XMMatrixRotationAxis(rotationAxisY, XMConvertToRadians(rotation));

			// The transformation matrices for the instances
			XMMATRIX transformation[3];
			transformation[0] = DirectX::XMMatrixIdentity();
			transformation[1] = DirectX::XMMatrixTranslation(-2, 0, 0) * rotationMat;
			transformation[2] = DirectX::XMMatrixTranslation(2, 0, 0) * rotationMat;

			// --------------------- TODO - CHECK ---------------------------
			// ------------------ ROW MAJOR MATRIX 3x4 ----------------------
			// --------------------- TODO - CHECK ---------------------------

			// The InstanceContributionToHitGroupIndex is set based on the shader-table layout specified in createShaderTable()
			// Create the desc for the triangle/plane instance
			instanceDescs[0].InstanceID = 0;
			instanceDescs[0].InstanceContributionToHitGroupIndex = 0;
			instanceDescs[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			memcpy(instanceDescs[0].Transform, &transformation[0], sizeof(instanceDescs[0].Transform));
			instanceDescs[0].AccelerationStructure = pBottomLevelAS[0]->GetGPUVirtualAddress();
			instanceDescs[0].InstanceMask = 0xFF;

			for (uint32_t i = 1; i < 3; i++)
			{
				instanceDescs[i].InstanceID = i; // This value will be exposed to the shader via InstanceID()
				instanceDescs[i].InstanceContributionToHitGroupIndex = (i * 2) + 2;  // The indices are relative to to the start of the hit-table entries specified in Raytrace(), so we need 4 and 6
				instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
				XMMATRIX m = XMMatrixTranspose(transformation[i]); // GLM is column major, the INSTANCE_DESC is row major
				memcpy(instanceDescs[i].Transform, &m, sizeof(instanceDescs[i].Transform));
				instanceDescs[i].AccelerationStructure = pBottomLevelAS[1]->GetGPUVirtualAddress();
				instanceDescs[i].InstanceMask = 0xFF; // Ray-Geometry intersections are processed when: (ray-mask__<fromShader_ArgOf_TraceRay> & InstanceMask__<fromTLAS> ) != 0

			}
		}
		// Unmap
		buffers.pInstanceDesc->Unmap(0, nullptr);
	}

	// Create the TLAS
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
		asDesc.Inputs = inputs;
		asDesc.Inputs.InstanceDescs = buffers.pInstanceDesc->GetGPUVirtualAddress();
		asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
		asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

		// If this is an update operation, set the source buffer and the perform_update flag
		if (update)
		{
			asDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			asDesc.SourceAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
		}

		pCmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);
	}

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	{
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = buffers.pResult.Get();
		pCmdList->ResourceBarrier(1, &uavBarrier);
	}
}

// =====================================================================================
//								Create RT PipelineState 
// =====================================================================================

ComPtr<ID3DBlob> CompileLibrary(const WCHAR* filename, const WCHAR* targetString)
{
	// Initialize the helper
	ThrowIfFailed(gDxcDllHelper.Initialize());
	ComPtr<IDxcCompiler> pCompiler;
	ComPtr<IDxcLibrary> pLibrary;
	ThrowIfFailed(gDxcDllHelper.CreateInstance( CLSID_DxcCompiler, pCompiler.GetAddressOf() ));
	ThrowIfFailed(gDxcDllHelper.CreateInstance( CLSID_DxcLibrary, pLibrary.GetAddressOf() ));

	// Open and read the file
	std::ifstream shaderFile(filename);
	if (shaderFile.good() == false)
	{
		MsgBox("Can't open file " + wstring_2_string(std::wstring(filename)));
		return nullptr;
	}
	std::stringstream strStream;
	strStream << shaderFile.rdbuf();
	std::string shader = strStream.str();

	// Create blob from the string
	ComPtr<IDxcBlobEncoding> pTextBlob;
	ThrowIfFailed(pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)shader.c_str(), (uint32_t)shader.size(), 0, &pTextBlob));

	// Compile
	ComPtr<IDxcOperationResult> pResult;
	ThrowIfFailed(pCompiler->Compile(pTextBlob.Get(), filename, L"", targetString, nullptr, 0, nullptr, 0, nullptr, &pResult));

	// Verify the result
	HRESULT resultCode;
	ThrowIfFailed(pResult->GetStatus(&resultCode));
	if (FAILED(resultCode))
	{
		ComPtr<IDxcBlobEncoding> pError;
		ThrowIfFailed(pResult->GetErrorBuffer(&pError));
		std::string log = convertBlobToString(pError.Get());
		MsgBox("Compiler error:\n" + log);
		return nullptr;
	}

	ComPtr<IDxcBlob> pBlob;
	ThrowIfFailed(pResult->GetResult(pBlob.GetAddressOf()));

	ComPtr<ID3DBlob> pD3dBlob = (ID3DBlob *) pBlob.Get();

	return pD3dBlob;
}

ComPtr<ID3D12RootSignature> createRootSignature(ComPtr<ID3D12Device5> pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	ComPtr<ID3DBlob> pSigBlob;
	ComPtr<ID3DBlob> pErrorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		std::string msg = convertBlobToString(pErrorBlob.Get());
		MsgBox(msg);
		return nullptr;
	}
	ComPtr<ID3D12RootSignature> pRootSig;
	ThrowIfFailed(pDevice->CreateRootSignature(0, pSigBlob->GetBufferPointer(), pSigBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSig)));
	return pRootSig;
}

struct RootSignatureDesc
{
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
};

RootSignatureDesc createRayGenRootDesc()
{
	// Create the root-signature
	RootSignatureDesc desc;
	desc.range.resize(2);
	// gOutput
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = 1;
	desc.range[0].RegisterSpace = 0;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	desc.range[0].OffsetInDescriptorsFromTableStart = 0;

	// gRtScene
	desc.range[1].BaseShaderRegister = 0;
	desc.range[1].NumDescriptors = 1;
	desc.range[1].RegisterSpace = 0;
	desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[1].OffsetInDescriptorsFromTableStart = 1;

	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 2;
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

	// Create the desc
	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}

RootSignatureDesc createTriangleHitRootDesc()
{
	RootSignatureDesc desc;
	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	desc.rootParams[0].Descriptor.RegisterSpace = 0;
	desc.rootParams[0].Descriptor.ShaderRegister = 0;

	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}

RootSignatureDesc createPlaneHitRootDesc()
{
	RootSignatureDesc desc;
	desc.range.resize(1);
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = 1;
	desc.range[0].RegisterSpace = 0;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[0].OffsetInDescriptorsFromTableStart = 0;

	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}

struct DxilLibrary
{
	DxilLibrary(ComPtr<ID3DBlob> pBlob, const WCHAR* entryPoint[], uint32_t entryPointCount) : pShaderBlob(pBlob)
	{
		stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		stateSubobject.pDesc = &dxilLibDesc;

		dxilLibDesc = {};
		exportDesc.resize(entryPointCount);
		exportName.resize(entryPointCount);
		if (pBlob)
		{
			dxilLibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
			dxilLibDesc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
			dxilLibDesc.NumExports = entryPointCount;
			dxilLibDesc.pExports = exportDesc.data();

			for (uint32_t i = 0; i < entryPointCount; i++)
			{
				exportName[i] = entryPoint[i];
				exportDesc[i].Name = exportName[i].c_str();
				exportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[i].ExportToRename = nullptr;
			}
		}
	};

	DxilLibrary() : DxilLibrary(nullptr, nullptr, 0) {}

	D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
	D3D12_STATE_SUBOBJECT stateSubobject{};
	ComPtr<ID3DBlob> pShaderBlob;
	std::vector<D3D12_EXPORT_DESC> exportDesc;
	std::vector<std::wstring> exportName;
};

static const WCHAR* kRayGenShader = L"rayGen";
static const WCHAR* kMissShader = L"miss";
static const WCHAR* kTriangleChs = L"triangleChs";
static const WCHAR* kPlaneChs = L"planeChs";
static const WCHAR* kTriHitGroup = L"TriHitGroup";
static const WCHAR* kPlaneHitGroup = L"PlaneHitGroup";
static const WCHAR* kShadowChs = L"shadowChs";
static const WCHAR* kShadowMiss = L"shadowMiss";
static const WCHAR* kShadowHitGroup = L"ShadowHitGroup";

DxilLibrary createDxilLibrary()
{
	// Compile the shader
	ComPtr<ID3DBlob> pRayGenShader = CompileLibrary(L"Shaders/14-Shaders.hlsl", L"lib_6_3");
	const WCHAR* entryPoints[] = { kRayGenShader, kMissShader, kPlaneChs, kTriangleChs, kShadowMiss, kShadowChs };
	return DxilLibrary(pRayGenShader, entryPoints, arraysize(entryPoints));
}

struct HitProgram
{
	HitProgram(LPCWSTR ahsExport, LPCWSTR chsExport, const std::wstring& name) : exportName(name)
	{
		desc = {};
		desc.AnyHitShaderImport = ahsExport;
		desc.ClosestHitShaderImport = chsExport;
		desc.HitGroupExport = exportName.c_str();

		subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subObject.pDesc = &desc;
	}

	std::wstring exportName;
	D3D12_HIT_GROUP_DESC desc;
	D3D12_STATE_SUBOBJECT subObject;
};

struct ExportAssociation
{
	ExportAssociation(const WCHAR* exportNames[], uint32_t exportCount, const D3D12_STATE_SUBOBJECT* pSubobjectToAssociate)
	{
		association.NumExports = exportCount;
		association.pExports = exportNames;
		association.pSubobjectToAssociate = pSubobjectToAssociate;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		subobject.pDesc = &association;
	}

	D3D12_STATE_SUBOBJECT subobject = {};
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
};

struct LocalRootSignature
{
	LocalRootSignature(ComPtr<ID3D12Device5> pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		pRootSig = createRootSignature(pDevice, desc);
		pInterface = pRootSig.Get();
		subobject.pDesc = &pInterface;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	}
	ComPtr<ID3D12RootSignature> pRootSig;
	ID3D12RootSignature* pInterface = nullptr;
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct GlobalRootSignature
{
	GlobalRootSignature(ComPtr<ID3D12Device5> pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		pRootSig = createRootSignature(pDevice, desc);
		pInterface = pRootSig.Get();
		subobject.pDesc = &pInterface;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	}
	ComPtr<ID3D12RootSignature> pRootSig;
	ID3D12RootSignature* pInterface = nullptr;
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct ShaderConfig
{
	ShaderConfig(uint32_t maxAttributeSizeInBytes, uint32_t maxPayloadSizeInBytes)
	{
		shaderConfig.MaxAttributeSizeInBytes = maxAttributeSizeInBytes;
		shaderConfig.MaxPayloadSizeInBytes = maxPayloadSizeInBytes;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		subobject.pDesc = &shaderConfig;
	}

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct PipelineConfig
{
	PipelineConfig(uint32_t maxTraceRecursionDepth)
	{
		config.MaxTraceRecursionDepth = maxTraceRecursionDepth;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		subobject.pDesc = &config;
	}

	D3D12_RAYTRACING_PIPELINE_CONFIG config = {};
	D3D12_STATE_SUBOBJECT subobject = {};
};

// =====================================================================================
//										Init 
// =====================================================================================

DxrGame::DxrGame(HINSTANCE hInstance, const wchar_t * windowTitle, int width, int height, bool vSync) :
	Application(hInstance, windowTitle, width, height, vSync),
	m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
	m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, (float)width, (float)height)),
	m_FoV(45.0f)
{
	// The first back buffer index will very likely be 0, but it depends
	m_CurrentBackBufferIndex = Application::GetCurrentBackbufferIndex();
}
DxrGame::~DxrGame()
{

}

// =====================================================================================
//										DXR-main
// =====================================================================================



void DxrGame::InitDXR()
{
	createAccelerationStructures();         
	createRtPipelineState();                
	createShaderResources();                
	createConstantBuffers();                
	createShaderTable();                    
}

void DxrGame::createAccelerationStructures()
{
	ComPtr<ID3D12Device5> device = Application::GetDevice();
	std::shared_ptr<CommandQueue> cmdQueue = Application::GetCommandQueue();
	ComPtr<ID3D12GraphicsCommandList4> cmdList = cmdQueue->GetCommandList();

	mpVertexBuffer[0] = CreateTriangleVB(device);
	mpVertexBuffer[1] = CreatePlaneVB(device);
	AccelerationStructureBuffers bottomLevelBuffers[2];

	// The first bottom-level buffer is for the plane and the triangle
	const uint32_t vertexCount[] = { 3, 6 };// Triangle has 3 vertices, plane has 6
	bottomLevelBuffers[0] = CreateBottomLevelAS(device, cmdList, mpVertexBuffer, vertexCount, 2);
	mpBottomLevelAS[0] = bottomLevelBuffers[0].pResult;

	// The second bottom-level buffer is for the triangle only
	bottomLevelBuffers[1] = CreateBottomLevelAS(device, cmdList, mpVertexBuffer, vertexCount, 1);
	mpBottomLevelAS[1] = bottomLevelBuffers[1].pResult;

	// Create the TLAS
	BuildTopLevelAS(device, cmdList, mpBottomLevelAS, mTlasSize, 0, false, mTopLevelBuffers);

	// The tutorial doesn't have any resource lifetime management, so we flush and sync here. This is not required by the DXR spec - you can submit the list whenever you like as long as you take care of the resources lifetime.
	//mFenceValue = submitCommandList(mpCmdList, mpCmdQueue, mpFence, mFenceValue);
	//mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
	//WaitForSingleObject(mFenceEvent, INFINITE);
	//mpCmdList->Reset(mFrameObjects[0].pCmdAllocator, nullptr);
	m_FenceValues[m_CurrentBackBufferIndex] = cmdQueue->ExecuteCommandList(cmdList);
	cmdQueue->WaitForFenceValue(m_FenceValues[m_CurrentBackBufferIndex]);
}

void DxrGame::createRtPipelineState() 
{

}

void DxrGame::createShaderResources()
{

}

void DxrGame::createConstantBuffers()
{

}

void DxrGame::createShaderTable()
{

}

// =====================================================================================
//						      LoadContent & UnloadContent
// =====================================================================================
void DxrGame::UpdateBufferResource(
	ComPtr<ID3D12GraphicsCommandList4> commandList,
	ID3D12Resource** pDestinationResource,
	ID3D12Resource** pIntermediateResource,
	size_t numElements, size_t elementSize, const void* bufferData,
	D3D12_RESOURCE_FLAGS flags)
{
	auto device = Application::GetDevice();

	size_t bufferSize = numElements * elementSize;

	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(pDestinationResource)));

	// Create a committed resource for the upload.
	if (bufferData)
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(commandList.Get(),
			*pDestinationResource, *pIntermediateResource,
			0, 0, 1, &subresourceData);
	}
}


bool DxrGame::LoadContent(std::wstring shaderBlobPath)
{
	auto device = Application::GetDevice();
	auto commandQueue = Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	// Upload vertex buffer data.
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	UpdateBufferResource(commandList,
		&m_VertexBuffer, &intermediateVertexBuffer,
		_countof(g_Vertices), sizeof(VertexPosColor), g_Vertices);

	// Create the vertex buffer view.
	m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = sizeof(g_Vertices);
	m_VertexBufferView.StrideInBytes = sizeof(VertexPosColor);

	// Upload index buffer data.
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UpdateBufferResource(commandList,
		&m_IndexBuffer, &intermediateIndexBuffer,
		_countof(g_Indicies), sizeof(WORD), g_Indicies);

	// Create index buffer view.
	m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_IndexBufferView.SizeInBytes = sizeof(g_Indicies);

	// Create the descriptor heap for the depth-stencil view.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));

	// Load the vertex shader.
	ComPtr<ID3DBlob> vertexShaderBlob;
	std::wstring blobPath = std::wstring(shaderBlobPath + L"VertexShader.cso");
	ThrowIfFailed(D3DReadFileToBlob(blobPath.c_str(), &vertexShaderBlob));

	// Load the pixel shader.
	ComPtr<ID3DBlob> pixelShaderBlob;
	blobPath = std::wstring(shaderBlobPath + L"PixelShader.cso");
	ThrowIfFailed(D3DReadFileToBlob(blobPath.c_str(), &pixelShaderBlob));

	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Create a root signature.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// A single 32-bit constant root parameter that is used by the vertex shader.
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	// Serialize the root signature.
	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
		featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
	// Create the root signature.
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = m_RootSignature.Get();
	pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineStateStream
	};
	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	m_ContentLoaded = true;

	// Resize/Create the depth buffer.
	ResizeDepthBuffer(Application::GetClientWidth(), Application::GetClientHeight());

	return true;
}


void DxrGame::UnloadContent() {
	m_ContentLoaded = false;
}

// =====================================================================================
//							  Update & Render & Resize
// =====================================================================================

void DxrGame::Update()
{
	Application::Update();
	double totalUpdateTime = Application::GetUpdateTotalTime();

	// Update the model matrix.
	float angle = static_cast<float>(totalUpdateTime * 90.0);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
	m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

	// Update the view matrix.
	const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Update the projection matrix.
	float aspectRatio = Application::GetClientWidth() / static_cast<float>(Application::GetClientHeight());
	m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.1f, 100.0f);
}

// Resources must be transitioned from one state to another using a resource BARRIER
//		and inserting that resource barrier into the command list.
// For example, before you can use the swap chain's back buffer as a render target, 
//		it must be transitioned to the RENDER_TARGET state and before it can be used
//		for presenting the rendered image to the screen, it must be transitioned to 
//		the  PRESENT state.
// 
//
// There are several types of resource barriers :
//	 - Transition: Transitions a(sub)resource to a particular state before using it. 
//			For example, before a texture can be used in a pixel shader, it must be 
//			transitioned to the PIXEL_SHADER_RESOURCE state.
//	 - Aliasing: Specifies that a resource is used in a placed or reserved heap when
//			that resource is aliased with another resource in the same heap.
//	 - UAV: Indicates that all UAV accesses to a particular resource have completed 
//			before any future UAV access can begin.This is necessary when the UAV is 
//			transitioned for :
//				* Read > Write: Guarantees that all previous read operations on the UAV
//						have completed before being written to in another shader.
//				* Write > Read: Guarantees that all previous write operations on the UAV
//						have completed before being read from in another shader.
//				* Write > Write: Avoids race conditions that could be caused by different
//						shaders in a different draw or dispatch trying to write to the same
//						resource(does not avoid race conditions that could be caused in the
//						same draw or dispatch call).
//				* A UAV barrier is not needed if the resource is being used as a 
//						read - only(Read > Read) resource between draw or dispatches.
void DxrGame::Render()
{
	Application::Render();
	double totalRenderTime = Application::GetRenderTotalTime();

	auto commandQueue = Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();

	m_CurrentBackBufferIndex = Application::GetCurrentBackbufferIndex();
	auto backBuffer = Application::GetBackbuffer(m_CurrentBackBufferIndex);

	auto rtv = Application::GetCurrentBackbufferRTV();
	auto dsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

	// Clear RT
	{
		TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		ClearRTV(commandList, rtv, clearColor);
		ClearDepth(commandList, dsv);
	}

	// Set Graphics state
	commandList->SetPipelineState(m_PipelineState.Get());
	commandList->SetGraphicsRootSignature(m_RootSignature.Get());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	commandList->IASetIndexBuffer(&m_IndexBufferView);

	commandList->RSSetViewports(1, &m_Viewport);
	commandList->RSSetScissorRects(1, &m_ScissorRect);

	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	// Update the MVP matrix
	XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

	// Draw
	commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);

	// PRESENT image
	{
		// After rendering the scene, the current back buffer is PRESENTed 
		//     to the screen.
		// !!! Before presenting, the back buffer resource must be 
		//     transitioned to the PRESENT state.
		TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		// Execute
		m_FenceValues[m_CurrentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

		m_CurrentBackBufferIndex = Application::Present();
		commandQueue->WaitForFenceValue(m_FenceValues[m_CurrentBackBufferIndex]);
	}
}

void DxrGame::Resize(UINT32 width, UINT32 height)
{
	if (Application::GetClientWidth() != width || Application::GetClientHeight() != height)
	{
		// RenderTargets
		{
			Application::Resize(width, height);

			// Since the index of back buffer may not be the same, it is important
			// to update the current back buffer index as known by the application.
			m_CurrentBackBufferIndex = Application::GetCurrentBackbufferIndex();
		}

		// Viewport and DephBuffer
		{
			m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
				static_cast<float>(width), static_cast<float>(height));

			ResizeDepthBuffer(width, height);
		}
	}
}

void DxrGame::ResizeDepthBuffer(int width, int height)
{
	if (m_ContentLoaded)
	{
		// Flush any GPU commands that might be referencing the depth buffer.
		Application::Flush();

		width = std::max(1, width);
		height = std::max(1, height);

		auto device = Application::GetDevice();

		// Resize screen dependent resources.
		// Create a depth buffer.
		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		optimizedClearValue.DepthStencil = { 1.0f, 0 };

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
				1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optimizedClearValue,
			IID_PPV_ARGS(&m_DepthBuffer)
		));

		// Update the depth-stencil view.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv,
			m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
	}
}

// =====================================================================================
//									Helper Funcs
// =====================================================================================

void DxrGame::TransitionResource(ComPtr<ID3D12GraphicsCommandList4> commandList, ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition
	(
		resource.Get(), before, after
	);

	commandList->ResourceBarrier(1, &barrier);
}

void DxrGame::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor)
{
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void DxrGame::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
{
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}