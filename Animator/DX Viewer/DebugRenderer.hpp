#pragma once

#include "MathTypes.hpp"

// Interface to the debug renderer
namespace MRenderer
{
	namespace DebugRenderer
	{

		void add_line(DirectX::XMFLOAT4 point_a, DirectX::XMFLOAT4 point_b, DirectX::XMFLOAT4 color_a, DirectX::XMFLOAT4 color_b);

		inline void add_line(DirectX::XMFLOAT4 p, DirectX::XMFLOAT4 q, DirectX::XMFLOAT4 color) { add_line(p, q, color, color); }

		void clear_lines();

		const Vertex* get_line_verts();

		size_t get_line_vert_count();

		size_t get_line_vert_capacity();
	}
}