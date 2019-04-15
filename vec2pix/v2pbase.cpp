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

	inline VFLOAT3 GetNDC(const VFLOAT4& posH)
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

	inline void PerspectiveCorrectInterpolation(
		VFLOAT3& q0, VFLOAT3& q1, VFLOAT3& q2, VFLOAT3& q01, VFLOAT3& q02, VFLOAT2& p2,
		const float& azh, const float& bzh, const float& czh,
		float& s, float& t, float& z_ndc
	)
	{
		calcST(q01.x, q02.x, q01.y, q02.y, p2.x - q0.x, p2.y - q0.y, s, t);
		float z_inv = (1.0f - s - t) * q0.z + s * q1.z + t * q2.z;		// 1 / -P'z = w' = z_inv
		s = s / z_inv * q1.z;
		t = t / z_inv * q2.z;
		// orth-z interpolation z_ndc = PHz * PHw = PHz / -Pz
		z_ndc = (1.0f - s - t) * azh + s * bzh + t * czh;
		z_ndc = z_ndc * z_inv;
	}

	template<typename T>
	inline T Interpolate(float s, float t, const T& a, const T& b, const T& c)
	{
		return (1 - s - t)*a + s * b + t * c;
	}

	void RasterizeTriangleByIndex(
		DEVICE *device,
		const vector<VERTEX>& vertexBuffer,
		const vector<uint16_t>& indexBuffer,
		const P_VMATRIX44& world2cameraM,
		const P_VMATRIX44& projectionM)
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
			// fillAndInterpolateTriangle(vertexBuffer, vertexPosH, fragmentBuffer, a, b, c, width, height);
			// primitive assembly
			const VERTEX& va = vertexBuffer[a];
			const VERTEX& vb = vertexBuffer[b];
			const VERTEX& vc = vertexBuffer[c];
			PRIMITIVE_VERTEX pa = { va.position, vertexPosH[a], va.color, va.normal, va.texCoord },
				pb = { vb.position, vertexPosH[b], vb.color, vb.normal, vb.texCoord },
				pc = { vc.position, vertexPosH[c], vc.color, vc.normal, vc.texCoord };
			// rasterization
			RasterizeTriangle(device, pa, pb, pc);
		}
	}

	void GetUVPartial(const DEVICE *device, VFLOAT3& q0, VFLOAT3& q1, VFLOAT3& q2, VFLOAT3& q01, VFLOAT3& q02, VFLOAT2& p,
		const PRIMITIVE_VERTEX& a, const PRIMITIVE_VERTEX& b, const PRIMITIVE_VERTEX& c, 
		float eps, VFLOAT2& duxy, VFLOAT2& dvxy)
	{
		float s, t, z_ndc, du, dv, epsx = eps * 2.0f;
		const float &azh = a.posH.z, &bzh = b.posH.z, &czh = c.posH.z;
		VFLOAT2 _p, tex_coord;

		// du/dx dv/dx
		_p = VFLOAT2(p.x + eps, p.y);
		PerspectiveCorrectInterpolation(q0, q1, q2, q01, q02, _p, azh, bzh, czh, s, t, z_ndc);
		tex_coord = Interpolate(s, t, a.texCoord, b.texCoord, c.texCoord);
		du = tex_coord.x;
		dv = tex_coord.y;
		_p = VFLOAT2(p.x - eps, p.y);
		PerspectiveCorrectInterpolation(q0, q1, q2, q01, q02, _p, azh, bzh, czh, s, t, z_ndc);
		tex_coord = Interpolate(s, t, a.texCoord, b.texCoord, c.texCoord);
		du = (du - tex_coord.x) / epsx / device->width;
		dv = (dv - tex_coord.y) / epsx / device->height;
		duxy.x = du;
		dvxy.x = dv;
		// - du/dy dv/dy
		_p = VFLOAT2(p.x, p.y + eps);
		PerspectiveCorrectInterpolation(q0, q1, q2, q01, q02, _p, azh, bzh, czh, s, t, z_ndc);
		tex_coord = Interpolate(s, t, a.texCoord, b.texCoord, c.texCoord);
		du = tex_coord.x;
		dv = tex_coord.y;
		_p = VFLOAT2(p.x, p.y - eps);
		PerspectiveCorrectInterpolation(q0, q1, q2, q01, q02, _p, azh, bzh, czh, s, t, z_ndc);
		tex_coord = Interpolate(s, t, a.texCoord, b.texCoord, c.texCoord);
		du = (du - tex_coord.x) / epsx / device->width;
		dv = (dv - tex_coord.y) / epsx / device->height;
		duxy.y = du;
		dvxy.y = dv;
	}

	void RasterizeTriangle(
		DEVICE *device,
		const PRIMITIVE_VERTEX& a, const PRIMITIVE_VERTEX& b, const PRIMITIVE_VERTEX& c
	)
	{
		long width = device->width, height = device->height;
		vector<FRAGMENT>& frag_buffer = device->frag_buffer;
		// a very clear & detailed interpolation solution: 
		// https://stackoverflow.com/questions/24441631/how-exactly-does-opengl-do-perspectively-correct-linear-interpolation
		
		float left, top, right, bottom;
		long leftI, topI, rightI, bottomI;
		// prepare interpolation
		// posH = [x y z w] in which w = -Pz
#define Q(v) VFLOAT3(v.x/v.w, v.y/v.w, 1.0f/v.w)
		VFLOAT3 q0 = Q(a.posH),
			q1 = Q(b.posH),
			q2 = Q(c.posH);
#undef Q
		VFLOAT3 q01 = q1 - q0, q02 = q2 - q0;
		VFLOAT2 a2 = { q0.x, q0.y }, b2 = { q1.x, q1.y }, c2 = { q2.x, q2.y };

		left = min3(q0.x, q1.x, q2.x) / 2.0f + 0.5f;
		right = max3(q0.x, q1.x, q2.x) / 2.0f + 0.5f;
		bottom = min3(q0.y, q1.y, q2.y) / 2.0f + 0.5f;
		top = max3(q0.y, q1.y, q2.y) / 2.0f + 0.5f;

		// get pixel space boundary
		leftI = max(0L, (long)(left * width));
		topI = min(height - 1, (long)(top * height));
		rightI = min(width - 1, (long)(right * width));
		bottomI = max(0L, (long)(bottom * height));

		// traverse pixels
		VFLOAT2 p2;
		FRAGMENT frag;
		float s, t;
		float z_ndc;
		for (long yI = bottomI; yI <= topI; ++yI)
		{
			for (long xI = leftI; xI <= rightI; ++xI)
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
				PerspectiveCorrectInterpolation(q0, q1, q2, q01, q02, p2, a.posH.z, b.posH.z, c.posH.z, s, t, z_ndc);
				// late z-test
				if (z_ndc < EPS || z_ndc > 1.0f) continue;

				frag.uv = VUINT2(xI, yI);
				frag.position = Interpolate(s, t, a.position, b.position, c.position);
				frag.color = Interpolate(s, t, a.color, b.color, c.color);
				frag.normal = Interpolate(s, t, a.normal, b.normal, c.normal);
				frag.texCoord = Interpolate(s, t, a.texCoord, b.texCoord, c.texCoord);
				frag.z = z_ndc;

				// Texture filtering du/dx dv/dx du/dy dv/dy
				if (device->texture_filter_method == TRILINEAR)
				{
					GetUVPartial(device, q0, q1, q2, q01, q02, p2, a, b, c, 1e-4, frag.duxy, frag.dvxy);
				}

				frag_buffer.push_back(frag);
			}
		}
	}
}
