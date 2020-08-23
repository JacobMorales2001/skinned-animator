#pragma once
#include <DirectXMath.h>

namespace MRenderer
{
	struct Vertex
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 normal;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 tex;
	};
}