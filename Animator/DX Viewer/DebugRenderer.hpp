#pragma once

#include "DirectXMath.h"

// Interface to the debug renderer
namespace MRenderer
{
	namespace DebugRenderer
	{
		struct ColoredVertex
		{
			DirectX::XMFLOAT3 pos;
			DirectX::XMFLOAT4 clr;
		};

		void add_line(DirectX::XMFLOAT3 point_a, DirectX::XMFLOAT3 point_b, DirectX::XMFLOAT4 color_a, DirectX::XMFLOAT4 color_b);

		inline void add_line(DirectX::XMFLOAT3 p, DirectX::XMFLOAT3 q, DirectX::XMFLOAT4 color) { add_line(p, q, color, color); }

		void clear_lines();

		const ColoredVertex* get_line_verts();

		size_t get_line_vert_count();

		size_t get_line_vert_capacity();
	}
}