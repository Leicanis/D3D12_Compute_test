#include "D3DUtils.h"


bool D3DUtils::InitDirect3D()
{
	CreateDXGIFactory2(0U, IID_PPV_ARGS(&dxgiFactory6));


	//----------------創建設備----------------
	dxgiFactory6->EnumAdapterByGpuPreference(
		0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&dxgiAdapter4));
	if (FAILED(D3D12CreateDevice(
		dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device8))))
	{
		cerr << "Create Device ERROR" << endl;
	}
	else
	{
		DXGI_ADAPTER_DESC3 deviceInfo{};
		dxgiAdapter4->GetDesc3(&deviceInfo);
		wcout << L"Adapter: " << deviceInfo.Description << L" " << endl;
		wcout << format(L"RAM: {:.2f} GB", (deviceInfo.DedicatedVideoMemory / 1000000000.f)) << endl;
		cout << endl;
	}
	return true;
}

bool D3DUtils::CreateCommittedResourceHeap(
	enum D3D12_HEAP_TYPE heapType, enum D3D12_RESOURCE_FLAGS resourceFlags,
	enum D3D12_RESOURCE_STATES resourceStates,
	UINT dataWidth, ComPtr<ID3D12Resource2>& d3d12Resource2)
{
	HRESULT hr = S_OK;

	D3D12_HEAP_PROPERTIES heapTypeProperties = { heapType };

	D3D12_RESOURCE_DESC d3d12ResourceDesc{};
	d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	d3d12ResourceDesc.Alignment = 0U;
	d3d12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	d3d12ResourceDesc.Flags = resourceFlags;
	d3d12ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	d3d12ResourceDesc.Width = dataWidth;
	d3d12ResourceDesc.Height = 1U;
	d3d12ResourceDesc.DepthOrArraySize = 1U;
	d3d12ResourceDesc.MipLevels = 1U;
	d3d12ResourceDesc.SampleDesc = { .Count = 1U, .Quality = 0U };

	if (FAILED(device8->CreateCommittedResource1(
		&heapTypeProperties, D3D12_HEAP_FLAG_NONE,
		&d3d12ResourceDesc, resourceStates,
		nullptr, nullptr, IID_PPV_ARGS(&d3d12Resource2))))
	{
		cerr << "Create Upload Heap ERROR " << endl;
		return false;
	}
	return true;
}

bool D3DUtils::CreateCBVSRVUAVDescriptorHeap(UINT descriptorCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.NumDescriptors = descriptorCount;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	if (FAILED(device8->CreateDescriptorHeap(
		&descriptorHeapDesc, IID_PPV_ARGS(&cbvsrvuavDescriptorHeap))))
	{
		cerr << "Create Descriptor Heap ERROR" << endl;
		return false;
	}
	return true;
}
void D3DUtils::CreateDescriptorView(
	enum DESCRIPTOR_TYPE descriptorType,
	UINT dataWidth, UINT dataByteStride, size_t IncrementCount,
	ComPtr<ID3D12Resource2>& d3d12Resource2)
{
	//----------------計算指針偏移量----------------
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
		cbvsrvuavDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	UINT descriptorSize = device8->
		GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle.ptr += descriptorSize * IncrementCount;

	if (descriptorType == DESCRIPTOR_TYPE::SRV)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvSourceDesc{};
		srvSourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvSourceDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvSourceDesc.Buffer.FirstElement = 0;
		srvSourceDesc.Buffer.NumElements = dataWidth / dataByteStride;
		srvSourceDesc.Buffer.StructureByteStride = dataByteStride;
		srvSourceDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvSourceDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		device8->CreateShaderResourceView(
			d3d12Resource2.Get(), &srvSourceDesc, cpuHandle);
	}
	else if (descriptorType == DESCRIPTOR_TYPE::UAV)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDestDesc{};
		uavDestDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDestDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDestDesc.Buffer.FirstElement = 0;
		uavDestDesc.Buffer.NumElements = dataWidth / dataByteStride;
		uavDestDesc.Buffer.StructureByteStride = dataByteStride;
		uavDestDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		device8->CreateUnorderedAccessView(
			d3d12Resource2.Get(), nullptr, &uavDestDesc, cpuHandle);
	}
	else if (descriptorType == DESCRIPTOR_TYPE::CBV)
	{

	}
}

bool D3DUtils::CreateRootSignatureWithDescriptorTable(
	const vector<pair<enum DESCRIPTOR_TYPE, UINT>>& descriptorRanges)
{
	//----------------創建所有描述符範圍----------------
	vector<D3D12_DESCRIPTOR_RANGE1> descriptorRangeList;
	for (const auto& range : descriptorRanges)
	{
		D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
		descriptorRange.RegisterSpace = 0;
		descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		if (range.first == DESCRIPTOR_TYPE::SRV)
		{
			descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			descriptorRange.NumDescriptors = range.second;
		}
		else if (range.first == DESCRIPTOR_TYPE::UAV)
		{
			descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			descriptorRange.NumDescriptors = range.second;
		}
		else if (range.first == DESCRIPTOR_TYPE::CBV)
		{
			descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			descriptorRange.NumDescriptors = range.second;
		}

		descriptorRangeList.push_back(descriptorRange);
	}

	//----------------創建所有根參數，並使用描述符範圍構造根簽名----------------
	vector<D3D12_ROOT_PARAMETER1> rootParameters;
	for (const auto& range : descriptorRangeList)
	{
		D3D12_ROOT_PARAMETER1 rootParam = {};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParam.DescriptorTable.NumDescriptorRanges = 1;
		rootParam.DescriptorTable.pDescriptorRanges = &range;

		rootParameters.push_back(rootParam);
	}

	//----------------創建根簽名描述----------------
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.Desc_1_1.NumParameters = static_cast<UINT>(rootParameters.size());
	rootSignatureDesc.Desc_1_1.pParameters = rootParameters.data();
	rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
	rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

	//----------------序列化根簽名----------------
	ComPtr<ID3DBlob> pSignatureBlob;
	ComPtr<ID3DBlob> pErrorBlob;
	if (FAILED(D3D12SerializeVersionedRootSignature(
		&rootSignatureDesc, &pSignatureBlob, &pErrorBlob)))
	{
		cerr << "Serialize VersionedRootSignature Error" << endl;
		if (pErrorBlob)
		{
			cerr << static_cast<char*>(pErrorBlob->GetBufferPointer()) << endl;
		}
		return false;
	}

	//----------------創建根簽名----------------
	HRESULT hr = device8->CreateRootSignature(
		0, pSignatureBlob->GetBufferPointer(),
		pSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	if (FAILED(hr)){
		cerr << "Create RootSignature Error" << hr << endl;
		return false;
	}
	return true;
}
bool D3DUtils::CompileShader(wstring shaderFileName, string entryPoint)
{
#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ComPtr<ID3DBlob> pErrorBlob;
	if (FAILED(D3DCompileFromFile(
		shaderFileName.c_str(), nullptr, nullptr, entryPoint.c_str(), "cs_5_1",
		compileFlags, 0, &d3d12ComputeShader, &pErrorBlob)))
	{
		cerr << "Compile Shader Error" << endl;
		if (pErrorBlob) {
			cerr << static_cast<char*>
				(pErrorBlob->GetBufferPointer()) << endl;
		}
		return false;
	}
	return true;
}
bool D3DUtils::CreatePSO()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12ComputePSODesc{};
	d3d12ComputePSODesc.pRootSignature = rootSignature.Get();
	d3d12ComputePSODesc.CS.pShaderBytecode =
		d3d12ComputeShader.Get()->GetBufferPointer();
	d3d12ComputePSODesc.CS.BytecodeLength =
		d3d12ComputeShader.Get()->GetBufferSize();
	d3d12ComputePSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	if (FAILED(device8->CreateComputePipelineState(
		&d3d12ComputePSODesc, IID_PPV_ARGS(&d3d12PipelineState))))
	{
		cerr << "Create ComputePipelineState Error" << endl;
		return false;
	}

	return true;
}
bool D3DUtils::CreateCommandSeries()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if (FAILED(device8->CreateCommandQueue(
		&commandQueueDesc, IID_PPV_ARGS(&commandQueue))))
	{
		cerr << "Create CommandQueue Error" << endl;
		return false;
	}

	if (FAILED(device8->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))))
	{
		cerr << "Create CommandAllocator Error" << endl;
		return false;
	}

	if (FAILED(device8->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
		IID_PPV_ARGS(&graphicsCommandList))))
	{
		cerr << "Create CommandList Error" << endl;
		return false;
	}

	if (FAILED(graphicsCommandList->Close()))
	{
		cerr << "Close CommandList Error" << endl;
		return false;
	}

	return true;
}

bool D3DUtils::SetupCommandList()
{
	//----------------Fill CommandList----------------
	if (FAILED(graphicsCommandList->Reset(commandAllocator.Get(), nullptr)))
	{
		cerr << "Reset CommandList ERROR" << endl;
		return false;
	}

	graphicsCommandList->SetComputeRootSignature(rootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { cbvsrvuavDescriptorHeap.Get() };
	graphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	D3D12_GPU_DESCRIPTOR_HANDLE cpuHandleForTable =
		cbvsrvuavDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	UINT descriptorSize = device8->
		GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	graphicsCommandList->SetComputeRootDescriptorTable(0, cpuHandleForTable);
	cpuHandleForTable.ptr += descriptorSize;
	graphicsCommandList->SetComputeRootDescriptorTable(1, cpuHandleForTable);

	graphicsCommandList->SetPipelineState(d3d12PipelineState.Get());

	return true;
}

D3D12_RESOURCE_BARRIER D3DUtils::CreateBarrier(
	enum D3D12_RESOURCE_STATES stateBefore, enum D3D12_RESOURCE_STATES stateAfter,
	ComPtr<ID3D12Resource2>& d3d12Resource2)
{
	D3D12_RESOURCE_BARRIER Barrier{};	//默認來源堆：初始狀態->複製目的地狀態
	Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	Barrier.Transition.pResource = d3d12Resource2.Get();
	Barrier.Transition.StateBefore = stateBefore;
	Barrier.Transition.StateAfter = stateAfter;
	Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	return Barrier;
}