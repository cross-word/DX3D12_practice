// fix :: class MeshGeometry, function exercise2::Draw(), exercise2::BuildBoxGeometry()
// ������ �κ� : Ŭ���� MeshGemoetry, �Լ� exercise2::Draw(), exercise2::BuildBoxGeometry()

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct VPosData
{
	XMFLOAT3 Pos;
};

struct VColorData
{
	XMFLOAT4 Color;
};

struct Vertex
{
	VPosData Pos;
	VColorData Color;
};

struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class exercise2 : public D3DApp
{
public:
	exercise2(HINSTANCE hInstance);
	exercise2(const exercise2& rhs) = delete;
	exercise2& operator=(const exercise2& rhs) = delete;
	~exercise2();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[2];

private:
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	//����� ���忡�� �޸� üũ��� Ŵ
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		exercise2 theApp(hInstance);
		if (!theApp.Initialize())
			return 0;
		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

exercise2::exercise2(HINSTANCE hInstance) : D3DApp(hInstance)
{

}

exercise2::~exercise2()
{

}

bool exercise2::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	//Ŀ�ǵ� ����Ʈ �ʱ�ȭ
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildBoxGeometry();
	BuildPSO();

	//Ŀ�ǵ� ����Ʈ�� �ִ� �ʱ�ȭ ��ɵ��� ����
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists); // Ŀ�ǵ� ť�� Ŀ�ǵ� �Ҵ��ڿ��� Ŀ�ǵ���� �Ѱ���

	//����� �Ϸ�� �� ���� ��ٸ��� (��ȿ����)
	FlushCommandQueue();

	return true;
}

void exercise2::OnResize()
{
	D3DApp::OnResize();

	//â�� ũ�Ⱑ �ٲ�����Ƿ� ��Ⱦ�� �����ϰ� ���� ����� �ٽ� ���
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f); //0.25f * MathHelper::Pi�� PIDIV4�� ���̴�?
	XMStoreFloat4x4(&mProj, P);
}

void exercise2::Update(const GameTimer& gt)
{
	//������ǥ�� ������ǥ�� ��ȯ
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	//�þ����
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero(); //Ÿ���� ���� ������?, null�� ���� ���� ������? ==> �����غ��� �ٸ� matrix�鵵 null�� �ƴѰ�찡 ����
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	//��� ���� ����
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	mObjectCB->CopyData(0, objConstants); // �� ���������� ���ϰ� ���� �ν��Ͻ��� ���� �����ϴ°���???
}

void exercise2::Draw(const GameTimer& gt)
{
	//��� �Ҵ��ڿ� ��� ��� �缳��
	ThrowIfFailed(mDirectCmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	//�ڿ� �뵵�� ���õ� ���� ���̸� Direct3D�� ����
	mCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//�ĸ� ���ۿ� ���� ���۸� �����
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr);

	//������ ����� ��ϵ� ���� ��� ���۵��� ����
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps); // ��� ���� �����Ҷ� ũ�⸦ �������� �����ʰ� �̸� ��������� �ϴ°� ���� ���

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 2, vertexBufferViews);
	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart()); // ??

	mCommandList->DrawIndexedInstanced(mBoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

	//���� ����
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//��ɵ��� ����� ��ģ��
	ThrowIfFailed(mCommandList->Close());

	//��� ������ ���� ��� ����� ��� ť�� �߰��Ѵ�
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//�ĸ� ���ۿ� ���� ���۸� ��ȯ
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	//��� ó�� ���(��ȿ��)
	FlushCommandQueue();
}

void exercise2::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(mhMainWnd);
}

void exercise2::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void exercise2::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		//��Ŭ�� -> ȸ��
		//���콺 �� �ȼ� �̵��� 1/4���� ����
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		//���� ����
		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		//��Ŭ�� -> �̵�
		//���콺 �� �ȼ� �̵��� ���� 0.005������ ����
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		//ī�޶� ������ ����
		mRadius += dx - dy;
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void exercise2::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void exercise2::BuildConstantBuffers()
{
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
	//���ۿ��� i��° ��ü�� ��� ������ �������� ��´�
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize; // �ε��� * �ڷ��ǹ���Ʈ�� => �ּ��̵�

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	md3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void exercise2::BuildRootSignature()
{
	//��Ʈ ������ ���̴� ���α׷��� ����ϴ� �ڿ����� �����Ѵ�

	//��Ʈ �Ű������� ������ ���̺��̰ų� ��Ʈ ������ �Ǵ� ��Ʈ ����̴�
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	//CBV �ϳ��� ��� ������ ���̺��� ����
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//��� ���� �ϳ��� ������ ������ ������ ����Ű�� ���� �ϳ��� �̷���� ��Ʈ ������ ����
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)
	));
}

void exercise2::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"D:\\DX3D12_practice\\ch6\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_0"); // �����η� ���Ƿ� ������
	mpsByteCode = d3dUtil::CompileShader(L"D:\\DX3D12_practice\\ch6\\Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

void exercise2::BuildBoxGeometry()
{
	std::array<VPosData, 8> positions = {
		XMFLOAT3(-1.0f, -1.0f, -1.0f),
		XMFLOAT3(-1.0f, +1.0f, -1.0f),
		XMFLOAT3(+1.0f, +1.0f, -1.0f),
		XMFLOAT3(+1.0f, -1.0f, -1.0f),
		XMFLOAT3(-1.0f, -1.0f, +1.0f),
		XMFLOAT3(-1.0f, +1.0f, +1.0f),
		XMFLOAT3(+1.0f, +1.0f, +1.0f),
		XMFLOAT3(+1.0f, -1.0f, +1.0f)
	};
	std::array<VColorData, 8> colors = {
		XMFLOAT4(Colors::White),
		XMFLOAT4(Colors::Black),
		XMFLOAT4(Colors::Red),
		XMFLOAT4(Colors::Green),
		XMFLOAT4(Colors::Blue),
		XMFLOAT4(Colors::Yellow),
		XMFLOAT4(Colors::Cyan),
		XMFLOAT4(Colors::Magenta)
	};

	std::array<Vertex, 8> vertices;

	for (int i = 0; i < positions.size(); i++)
	{
		vertices[i].Pos = positions[i];
		vertices[i].Color = colors[i];
	}

	std::array<std::uint16_t, 36> indices =
	{
		//�ո�
		0, 1, 2,
		0, 2, 3,

		//�޸�
		4, 6, 5,
		4, 7, 6,

		//���ʸ�
		4, 5, 1,
		4, 1, 0,

		//������ ��
		3, 2, 6,
		3, 6, 7,

		//����
		1, 5, 6,
		1, 6, 2,

		//�Ʒ���
		4, 0, 3,
		4, 3, 7
	};

	const UINT vp_bByteSize = (UINT)positions.size() * sizeof(VPosData);
	const UINT vc_bByteSize = (UINT)colors.size() * sizeof(VColorData);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vp_bByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), positions.data(), vp_bByteSize);

	ThrowIfFailed(D3DCreateBlob(vc_bByteSize, &mBoxGeo->ColorVertexBufferCPU));
	CopyMemory(mBoxGeo->ColorVertexBufferCPU->GetBufferPointer(), colors.data(), vc_bByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(), mCommandList.Get(), positions.data(), vp_bByteSize, mBoxGeo->VertexBufferUploader);
	mBoxGeo->VertexByteStride = sizeof(VPosData);
	mBoxGeo->VertexBufferByteSize = vp_bByteSize;
	vertexBufferViews[0] = mBoxGeo->VertexBufferView();

	mBoxGeo->ColorVertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(), mCommandList.Get(), colors.data(), vc_bByteSize, mBoxGeo->ColorVertexBufferUploader);
	mBoxGeo->ColorVertexByteStride = sizeof(VColorData);
	mBoxGeo->ColorVertexBufferByteSize = vc_bByteSize;
	vertexBufferViews[1] = mBoxGeo->ColorVertexBufferView();


	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader); // bufferpointer�� �޸� �����س��� �� .data()�� ������???

	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;
}

void exercise2::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}