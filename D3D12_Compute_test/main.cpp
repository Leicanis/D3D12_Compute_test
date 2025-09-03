#include "Include.h"

#include "D3DUtils.h"

UINT sourceData[] = { 1, 1 };
UINT readbackData[] = { 0 };

int main()
{
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		debugController->EnableDebugLayer();
	else
		cout << "Debug Layer ERROR" << endl;


	ComPtr<ID3D12Resource2> d3d12UploadDataResource;
	ComPtr<ID3D12Resource2> d3d12DefaultSourceDataResource;
	ComPtr<ID3D12Resource2> d3d12DefaultDestDataResource;
	ComPtr<ID3D12Resource2> d3d12ReadbackDataResource;

	D3DUtils d3dUtils;

	//----------------InitDevice----------------
	d3dUtils.InitDirect3D();

	//----------------CreateHeap----------------
	d3dUtils.CreateCommittedResourceHeap(
		D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_COMMON, sizeof(sourceData), d3d12UploadDataResource);
	d3dUtils.CreateCommittedResourceHeap(
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON, sizeof(sourceData), d3d12DefaultSourceDataResource);
	d3dUtils.CreateCommittedResourceHeap(
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON, sizeof(readbackData), d3d12DefaultDestDataResource);
	d3dUtils.CreateCommittedResourceHeap(
		D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_COMMON, sizeof(readbackData), d3d12ReadbackDataResource);

	//----------------CreateDescriptorHeap----------------
	d3dUtils.CreateCBVSRVUAVDescriptorHeap(2);

	//----------------CreateDescriptor----------------
	d3dUtils.CreateDescriptorView(
		SRV, sizeof(sourceData), sizeof(sourceData[0]), 0, d3d12UploadDataResource);
	d3dUtils.CreateDescriptorView(
		UAV, sizeof(readbackData), sizeof(readbackData[0]), 1, d3d12DefaultDestDataResource);

	//----------------CreateRootSignature----------------
	std::vector<std::pair<enum DESCRIPTOR_TYPE, UINT>> descriptorRanges = {
		{DESCRIPTOR_TYPE::SRV, 1},
		{DESCRIPTOR_TYPE::UAV, 1} };
	d3dUtils.CreateRootSignatureWithDescriptorTable(descriptorRanges);

	//----------------CompileShader----------------
	d3dUtils.CompileShader(L"Shader\\add.hlsl", "CSMain");

	//----------------CreatePSO----------------
	d3dUtils.CreatePSO();

	//----------------CreateCommandSeries----------------
	d3dUtils.CreateCommandSeries();

	//----------------CreateBarrier----------------
	D3D12_RESOURCE_BARRIER d3d12DefaultSourceBarrier1 = d3dUtils.CreateBarrier(
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST,
		d3d12DefaultSourceDataResource);
	D3D12_RESOURCE_BARRIER d3d12DefaultSourceBarrier2 = d3dUtils.CreateBarrier(
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON,
		d3d12DefaultSourceDataResource);

	D3D12_RESOURCE_BARRIER d3d12DefaultDestBarrier1 = d3dUtils.CreateBarrier(
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		d3d12DefaultDestDataResource);
	D3D12_RESOURCE_BARRIER d3d12DefaultDestBarrier2 = d3dUtils.CreateBarrier(
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE,
		d3d12DefaultDestDataResource);

	//----------------Upload UploadBuffer----------------
	int* uploadDataBuffer = nullptr;
	D3D12_RANGE uploadDataRange = { 0, 0 };

	if (FAILED(d3d12UploadDataResource->Map(
		0, &uploadDataRange, reinterpret_cast<void**>(&uploadDataBuffer))))
	{
		cout << "Map ERROR" << endl;
	}
	memcpy(uploadDataBuffer, sourceData, sizeof(sourceData));
	d3d12UploadDataResource->Unmap(0, nullptr);

	//----------------Fill CommandList----------------
	d3dUtils.SetupCommandList();

	d3dUtils.GetCommandList()->ResourceBarrier(1, &d3d12DefaultSourceBarrier1);
	d3dUtils.GetCommandList()->CopyResource(
		d3d12DefaultSourceDataResource.Get(), d3d12UploadDataResource.Get());
	d3dUtils.GetCommandList()->ResourceBarrier(1, &d3d12DefaultSourceBarrier2);
	d3dUtils.GetCommandList()->ResourceBarrier(1, &d3d12DefaultDestBarrier1);
	d3dUtils.GetCommandList()->Dispatch(1, 1, 1);
	d3dUtils.GetCommandList()->ResourceBarrier(1, &d3d12DefaultDestBarrier1);
	d3dUtils.GetCommandList()->CopyResource(
		d3d12ReadbackDataResource.Get(), d3d12DefaultDestDataResource.Get());

	if (FAILED(d3dUtils.GetCommandList()->Close()))
	{
		cerr << "Close CommandList ERROR!" << endl;
	}
	ID3D12CommandList* ppCommandLists[] = { d3dUtils.GetCommandList().Get() };
	d3dUtils.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


	//----------------Read ReadbackBuffer----------------
	int* readbackDataBuffer = nullptr;
	D3D12_RANGE readbackDataRange = { 0, 0 };

	if (FAILED(d3d12ReadbackDataResource->Map(
		0, &readbackDataRange, reinterpret_cast<void**>(&readbackDataBuffer))))
	{
		cout << "Map ERROR!" << endl;
	}

	cout << sourceData[0] << " + " << sourceData[1] << " = " << readbackDataBuffer[0] << endl;
	d3d12ReadbackDataResource->Unmap(0, nullptr);

	//----------------End----------------
	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue = 1;
	if (FAILED(d3dUtils.GetDevice()->
		CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
	{
		cout << "Create Fence ERROR!" << endl;
	}

	d3dUtils.GetCommandQueue()->Signal(fence.Get(), fenceValue);

	if (fence->GetCompletedValue() < fenceValue) {
		HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (eventHandle == 0)
			return 0;
		fence->SetEventOnCompletion(fenceValue, eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}