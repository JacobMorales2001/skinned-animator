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
		DirectX::XMINT4 joints;
		DirectX::XMFLOAT4 weights;
	};

	static DirectX::XMFLOAT4 Float4Lerp(const DirectX::XMFLOAT4& a, const DirectX::XMFLOAT4& b, float t)
	{
		DirectX::XMFLOAT4 r;
		r.x = (b.x - a.x) * t + a.x;
		r.y = (b.y - a.y) * t + a.y;
		r.z = (b.z - a.z) * t + a.z;
		r.w = (b.w - a.w) * t + a.w;
		return r;
	}

	static DirectX::XMFLOAT4 Float4Add(const DirectX::XMFLOAT4& a, const DirectX::XMFLOAT4& b)
	{
		DirectX::XMFLOAT4 r;
		r.x = b.x + a.x;
		r.y = b.y + a.y;
		r.z = b.z + a.z;
		r.w = b.w + a.w;
		return r;
	}

	static DirectX::XMFLOAT4 Float4MultiplyFloat(const DirectX::XMFLOAT4& a, const  float& b)
	{
		DirectX::XMFLOAT4 r;
		r.x = b * a.x;
		r.y = b * a.y;
		r.z = b * a.z;
		r.w = b * a.w;
		return r;
	}

	namespace Color
	{
		constexpr DirectX::XMFLOAT4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
		constexpr DirectX::XMFLOAT4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
		constexpr DirectX::XMFLOAT4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
		constexpr DirectX::XMFLOAT4 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
		constexpr DirectX::XMFLOAT4 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
		constexpr DirectX::XMFLOAT4 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };
		constexpr DirectX::XMFLOAT4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
		constexpr DirectX::XMFLOAT4 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
		constexpr DirectX::XMFLOAT4 Gray = { 0.5f, 0.5f, 0.5f, 1.0f };
	}
}