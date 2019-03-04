#ifndef V2PBASE_H
#define V2PBASE_H

#include <memory>
#include <vector>
#include "v2pmath.h"

using namespace std;

namespace v2p
{
	/********************************
	Base vertex and fragment
	********************************/

	// Base vertex
	struct VERTEX
	{
		VFLOAT3 position;
		VFLOAT3 color;
		VERTEX() = default;
		VERTEX(const VERTEX&) = default;
		VERTEX& operator=(const VERTEX&) = default;
		VERTEX(VERTEX&&) = default;
		VERTEX& operator=(VERTEX&&) = default;
	};

	// Base fragment
	struct FRAGMENT
	{
		VFLOAT3 position;
		VFLOAT3 color;
		float w;
		FRAGMENT() = default;
		FRAGMENT(const FRAGMENT&) = default;
		FRAGMENT& operator=(const FRAGMENT&) = default;
		FRAGMENT(FRAGMENT&&) = default;
		FRAGMENT& operator=(FRAGMENT&&) = default;
	};

	/**
	Rasterize triangles. Project 3d vertexes into homogeneous clip space.
	Rasterize triangles, interpolate attributes and output to fragmentBuffer.
	Args:
	vertexBuffer   - A vector that contains vertex info.
	indexBuffer    - A vector that contains index info. Every 3 indexes represents a triangle.
	fragmentBuffer - Output buffer.
	world2cameraM  - World to camera space matrix44.
	projectionM    - Camera space to homogenous clip space matrix44.
	width          - Integer that represents output device width.
	height         - Integer that represents output device height.
	**/
	void rasterizeTriangleByIndex(
		const vector<VERTEX>& vertexBuffer, 
		const vector<uint16_t>& indexBuffer, 
		vector<FRAGMENT>& fragmentBuffer,
		const P_VMATRIX44& world2cameraM,
		const P_VMATRIX44& projectionM,
		uint16_t width, uint16_t height);

}


#endif