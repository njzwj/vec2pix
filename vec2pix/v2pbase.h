#ifndef V2PBASE_H
#define V2PBASE_H

#include <memory>
#include <vector>
#include "v2pmath.h"

using namespace std;

namespace v2p
{
	/********************************
	Array Buffer
	********************************/
	struct ARRAY_BUFFER_DESC
	{
		const char *name;
		enum ARRAY_BUFFER_USAGE
		{
			POSITION = 0,
			POSITION_H,
			NORMAL,
			TEX_COORD,
			COLOR,
			INDEX
		} usage;
		size_t offset;
		size_t size;

		ARRAY_BUFFER_DESC() = default;
		ARRAY_BUFFER_DESC(const ARRAY_BUFFER_DESC&) = default;
		ARRAY_BUFFER_DESC& operator=(const ARRAY_BUFFER_DESC&) = default;
		ARRAY_BUFFER_DESC(ARRAY_BUFFER_DESC&&) = default;
		ARRAY_BUFFER_DESC& operator=(ARRAY_BUFFER_DESC&&) = default;
	};

	struct ARRAY_BUFFER
	{
		uint8_t *p;
		size_t size;
		ARRAY_BUFFER() :p(nullptr), size(0) {}
		ARRAY_BUFFER(size_t _size) :p(nullptr), size(_size)
		{
			p = new uint8_t[_size];
		}
		~ARRAY_BUFFER()
		{
			delete[] p;
		}

		void expand(size_t expansion)
		{
			uint8_t *p1 = new uint8_t[size + expansion];
			if (p != nullptr)
			{
				memcpy(p1, p, size);
				delete[] p;
			}
			p = p1;
			size += expansion;
		}
		void add(const void* _src, const ARRAY_BUFFER_DESC& _desc)
		{
			memcpy(p + _desc.offset, _src, _desc.size);
		}
	};


	/********************************
	Base vertex and fragment
	********************************/
	// Base vertex
	struct VERTEX
	{
		VFLOAT3 position;
		VFLOAT3 color;
		VFLOAT2 texCoord;
		VFLOAT3 normal;
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
		VFLOAT3 normal;
		VFLOAT2 texCoord;
		VUINT2 uv;
		float z;
		FRAGMENT() = default;
		FRAGMENT(const FRAGMENT&) = default;
		FRAGMENT& operator=(const FRAGMENT&) = default;
		FRAGMENT(FRAGMENT&&) = default;
		FRAGMENT& operator=(FRAGMENT&&) = default;
	};

	// Base 2D Buffer
	template<typename T>
	class BUFFER2D
	{
	public:
		unique_ptr<T[]> buffer;

		BUFFER2D(uint16_t _w, uint16_t _h) :
			width(_w), height(_h)
		{
			this->buffer = unique_ptr<T[]>(new T[_w * _h * sizeof(T)]);
		}
		void fill(T x)
		{
			for (size_t i = 0; i < width * height * sizeof(T); ++i)
				this->buffer[i] = x;
		}
		uint16_t getWidth() { return this->width; }
		uint16_t getHeight() { return this->height; }
		void setBuffer(uint16_t x, uint16_t y, T buf) 
		{
			uint32_t p = (uint32_t)y * width + x;
			this->buffer[p] = buf;
		}
		T getBuffer(uint16_t x, uint16_t y)
		{
			uint32_t p = (uint32_t)y * width + x;
			return this->buffer[p];
		}
	private:
		uint16_t width;
		uint16_t height;
	};

	/**
	Rasterize triangles. Project 3d vertices into homogeneous clip space.
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
	void RasterizeTriangleByIndex(
		const vector<VERTEX>& vertexBuffer, 
		const vector<uint16_t>& indexBuffer, 
		vector<FRAGMENT>& fragmentBuffer,
		const P_VMATRIX44& world2cameraM,
		const P_VMATRIX44& projectionM,
		uint16_t width, uint16_t height);


	/************************
	Primitives and rasterization
	*************************/
	struct PRIMITIVE_VERTEX
	{
		VFLOAT3 position;
		VFLOAT4 posH;
		VFLOAT3 color;
		VFLOAT3 normal;
		VFLOAT2 texCoord;
	};

	/**
	Rasterize traiangle from preimitive
	Args:
	a, b, c          - Vertices of premitives.
	frag_buffer      - Vector of fragment buffer.
	width            - Output device width.
	height           - Output device height.
	**/
	void rasterizeTriangle(
		const PRIMITIVE_VERTEX& a, const PRIMITIVE_VERTEX& b, const PRIMITIVE_VERTEX& c,
		vector<FRAGMENT>& frag_buffer,
		uint16_t width, uint16_t height
	);
}


#endif
