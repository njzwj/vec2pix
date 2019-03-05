#include "v2pbase.h"
#include <algorithm>

using namespace std;

namespace v2p
{

	template<typename T>
	inline T min3(const T& a, const T& b, const T& c)
	{
		T ans = a;
		if (b < ans) ans = b;
		if (c < ans) return c;
		return ans;
	}

	template<typename T>
	inline T max3(const T& a, const T& b, const T& c)
	{
		T ans = a;
		if (b > ans) ans = b;
		if (c > ans) return c;
		return ans;
	}

	inline VFLOAT3 getNDC(const VFLOAT4& posH)
	{
		return VFLOAT3(posH.x / posH.w, posH.y / posH.w, posH.z / posH.w + EPS);
	}

	inline void calcST(const float& a, const float& b, const float& c, const float& d, 
		const float& e, const float& f, float& s, float& t)
	{
		/**
		| a  b |   | s |   | e |
		|      | x |   | = |   |
		| c  d |   | t |   | f |
		s = (de - bf) / D
		t = (af - ce) / D
		**/
		float D = a * d - b * c;
		s = (d * e - b * f) / D;
		t = (a * f - c * e) / D;
	}

	void fillAndInterpolateTriangle(
		const vector<VERTEX>& vertexBuffer,
		const vector<VFLOAT4>& vertexPosH,
		vector<FRAGMENT>& fragmentBuffer,
		uint16_t a, uint16_t b, uint16_t c,
		uint16_t width, uint16_t height
	)
	{
		// a very clear & detailed interpolation solution: 
		// https://stackoverflow.com/questions/24441631/how-exactly-does-opengl-do-perspectively-correct-linear-interpolation
		
		// bounding box & vertex preprocessing
		float left, top, right, bottom;
		uint16_t leftI, topI, rightI, bottomI;

#define UV(v) VFLOAT2(v.x / v.w, v.y / v.w)
		VFLOAT2 a2 = UV(vertexPosH[a]),
			b2 = UV(vertexPosH[b]),
			c2 = UV(vertexPosH[c]);
#undef UV
		left = min3(a2.x, b2.x, c2.x) / 2.0f + 0.5f;
		right = max3(a2.x, b2.x, c2.x) / 2.0f + 0.5f;
		bottom = min3(a2.y, b2.y, c2.y) / 2.0f + 0.5f;
		top = max3(a2.y, b2.y, c2.y) / 2.0f + 0.5f;

		// get pixel space boundary
		leftI = max((uint16_t)0, static_cast<uint16_t>(left * width));
		topI = max((uint16_t)0, static_cast<uint16_t>(top * height));
		rightI = min((uint16_t)(width - 1), static_cast<uint16_t>(right * width));
		bottomI = min((uint16_t)(height - 1), static_cast<uint16_t>(bottom * height));

		// prepare interpolation
		// posH = [x y z w] in which w = -Pz
#define Q(v) VFLOAT3(v.x/v.w, v.y/v.w, 1.0f/v.w)
		VFLOAT3 q0 = Q(vertexPosH[a]),
			q1 = Q(vertexPosH[b]),
			q2 = Q(vertexPosH[c]);
#undef Q
		VFLOAT3 q01 = q1 - q0, q02 = q2 - q0;

		// traverse pixels
		VFLOAT2 p2;
		FRAGMENT frag;
		float s, t;
		float z_inv, z_ndc;
		for (uint16_t yI = bottomI; yI <= topI; ++yI)
		{
			for (uint16_t xI = leftI; xI <= rightI; ++xI)
			{
				float x = (float)xI / width * 2.0f - 1.0f;
				float y = (float)yI / height * 2.0f - 1.0f;
				p2 = VFLOAT2(x, y);

				// triangle test
				if (lineTest(a2, b2, p2) == 0 ||
					lineTest(b2, c2, p2) == 0 ||
					lineTest(c2, a2, p2) == 0)
				{
					continue;
				}
				
				// perspective-correct interpolation
				calcST(q01.x, q02.x, q01.y, q02.y, p2.x - q0.x, p2.y - q0.y, s, t);
				z_inv = (1.0f - s - t) * q0.z + s * q1.z + t * q2.z;		// 1 / -P'z = w' = z_inv
				s = s / z_inv * q1.z;
				t = t / z_inv * q2.z;

				// orth-z interpolation z_ndc = PHz * PHw = PHz / -Pz
				z_ndc = (1.0f - s - t) * vertexPosH[a].z + s * vertexPosH[b].z + t * vertexPosH[c].z;
				z_ndc = z_ndc * z_inv;
				// late z-test
				if (z_ndc < EPS || z_ndc > 1.0f)
				{
					continue;
				}

				frag.uv = VUINT2(xI, yI);
				frag.position = 
					(1.0f - s - t) * vertexBuffer[a].position + 
					s * vertexBuffer[b].position + 
					t * vertexBuffer[c].position;
				frag.color = 
					(1.0f - s - t) * vertexBuffer[a].color +
				 	s * vertexBuffer[b].color + 
					t * vertexBuffer[c].color;
				/*
				if (s < 1.0f && s > 0.0f && t < 1.0f && t > 0.0f)
				{
					frag.color = { 1.0f, 1.0f, 1.0f };
				}
				else
				{
					frag.color = { 0.0f, 1.0f, 1.0f };
				}
				*/
				frag.z = z_ndc;

				fragmentBuffer.push_back(frag);
			}
		}
	}

	void rasterizeTriangleByIndex(
		const vector<VERTEX>& vertexBuffer,
		const vector<uint16_t>& indexBuffer,
		vector<FRAGMENT>& fragmentBuffer,
		const P_VMATRIX44& world2cameraM,
		const P_VMATRIX44& projectionM,
		uint16_t width, uint16_t height)
	{
		// Project vertex to homogeneous clip space
		P_VMATRIX44 tMat = matMul(projectionM, world2cameraM);
		vector<VFLOAT4> vertexPosH;
		vector<bool> vertexClip;	// early z clip, if z < 0 then clip
		for (auto x : vertexBuffer)
		{
			VFLOAT4 posH = tMat->mulVec3(x.position);
			vertexPosH.push_back(posH);
			vertexClip.push_back(posH.w < 0 ? true : false);
		}
		
		// Traverse triangles
		for (uint16_t i = 0; i < indexBuffer.size(); i += 3)
		{
			uint16_t a, b, c;
			a = indexBuffer[i];
			b = indexBuffer[i + 1];
			c = indexBuffer[i + 2];
			// early z test
			if (vertexClip[a] || vertexClip[b] || vertexClip[c]) continue;
			fillAndInterpolateTriangle(vertexBuffer, vertexPosH, fragmentBuffer, a, b, c, width, height);
		}
	}

}