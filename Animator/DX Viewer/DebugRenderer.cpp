#include "DebugRenderer.hpp"
#include <array>

// Anonymous namespace
namespace
{
	// Declarations in an anonymous namespace are global BUT only have internal linkage.
	// In other words, these variables are global but are only visible in this source file.

	// Maximum number of debug lines at one time (i.e: Capacity)
	constexpr size_t MAX_LINE_VERTS = 4096;

	// CPU-side buffer of debug-line verts
	// Copied to the GPU and reset every frame.
	size_t line_vert_count = 0;
	std::array< MRenderer::Vertex, MAX_LINE_VERTS> line_verts;
}

namespace MRenderer
{
	namespace DebugRenderer
	{
		void add_line(DirectX::XMFLOAT4 point_a, DirectX::XMFLOAT4 point_b, DirectX::XMFLOAT4 color_a, DirectX::XMFLOAT4 color_b)
		{
			// Add points to debug_verts, increments debug_vert_count
			line_verts[line_vert_count] = { point_a, {0.0f, 0.0f, 0.0f, 0.0f}, color_a, {0.0f, 0.0f} };
			line_verts[line_vert_count + 1] = { point_b, {0.0f, 0.0f, 0.0f, 0.0f}, color_b, {0.0f, 0.0f} };
			line_vert_count += 2;
		}

		void clear_lines()
		{
			// Resets debug_vert_count
			line_vert_count = 0;
		}

		const Vertex* get_line_verts()
		{
			// Does just what it says in the name
			return line_verts.data();
		}

		size_t get_line_vert_count()
		{
			// Does just what it says in the name
			return line_vert_count;
		}

		size_t get_line_vert_capacity()
		{
			// Does just what it says in the name
			return MAX_LINE_VERTS;
		}
	}
}