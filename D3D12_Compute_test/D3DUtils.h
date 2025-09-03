#pragma once

#include "Include.h"


typedef
enum DESCRIPTOR_TYPE
{
	SRV,
	UAV,
	CBV
}DESCRIPTOR_TYPE;

class D3DUtils
{
private:
	ComPtr<IDXGIFactory6> dxgiFactory6;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	ComPtr<ID3D12Device8> device8;

	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> graphicsCommandList;

	ComPtr<ID3D12DescriptorHeap> cbvsrvuavDescriptorHeap;
	ComPtr<ID3D12RootSignature> rootSignature;

	ComPtr<ID3DBlob> d3d12ComputeShader;
	ComPtr<ID3D12PipelineState> d3d12PipelineState;

	ComPtr<ID3D12Fence1> d3d12Fence1;

public:
	ComPtr<ID3D12Device8>& GetDevice() { return device8; }
	ComPtr<ID3D12CommandQueue>& GetCommandQueue() { return commandQueue; }
	ComPtr<ID3D12CommandAllocator>& GetCommandAllocator() { return commandAllocator; }
	ComPtr<ID3D12GraphicsCommandList>& GetCommandList() { return graphicsCommandList; }

public:
	bool InitDirect3D();

	bool CreateCommittedResourceHeap(
		enum D3D12_HEAP_TYPE heapType, enum D3D12_RESOURCE_FLAGS resourceFlags, 
		enum D3D12_RESOURCE_STATES resourceStates,
		UINT dataWidth, ComPtr<ID3D12Resource2>& d3d12Resource2);

	bool CreateCBVSRVUAVDescriptorHeap(UINT descriptorCount);
	void CreateDescriptorView(
		enum DESCRIPTOR_TYPE descriptorType,
		UINT dataWidth, UINT dataByteStride, size_t IncrementCount,
		ComPtr<ID3D12Resource2>& d3d12Resource2);

	bool CreateRootSignatureWithDescriptorTable(
		const std::vector<std::pair<enum DESCRIPTOR_TYPE, UINT>>& descriptorRanges);
	bool CompileShader(wstring shaderFileName, string entryPoint);
	bool CreatePSO();
	bool CreateCommandSeries();

	bool SetupCommandList();

	D3D12_RESOURCE_BARRIER CreateBarrier(
		enum D3D12_RESOURCE_STATES stateBefore, enum D3D12_RESOURCE_STATES stateAfter,
		ComPtr<ID3D12Resource2>& d3d12Resource2);

};