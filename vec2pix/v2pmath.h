#ifndef V2PMATH_H
#define V2PMATH_H

#include <stdint.h>
#include <memory>

using namespace std;

namespace v2p
{
	/************************************************************************************
	Type definitions
	************************************************************************************/
	// Vec 2f
	struct VFLOAT2
	{
		float x;
		float y;
		VFLOAT2() = default;
		VFLOAT2(const VFLOAT2&) = default;
		VFLOAT2& operator=(const VFLOAT2&) = default;
		VFLOAT2(VFLOAT2&&) = default;
		VFLOAT2& operator=(VFLOAT2&&) = default;

		VFLOAT2(float _x, float _y) :x(_x), y(_y) {}

		VFLOAT2 operator + (const VFLOAT2& rhs) const { return VFLOAT2(x + rhs.x, y + rhs.y); }
		VFLOAT2 operator - (const VFLOAT2& rhs) const { return VFLOAT2(x - rhs.x, y - rhs.y); }
		VFLOAT2 operator * (const VFLOAT2& rhs) const { return VFLOAT2(x * rhs.x, y * rhs.y); }
		VFLOAT2 operator * (float rhs) const { return VFLOAT2(x * rhs, y * rhs); }
		VFLOAT2 operator / (float rhs) const { return VFLOAT2(x / rhs, y / rhs); }
	};
	// Vec 3f
	struct VFLOAT3
	{
		float x;
		float y;
		float z;
		VFLOAT3() = default;
		VFLOAT3(const VFLOAT3&) = default;
		VFLOAT3& operator=(const VFLOAT3&) = default;
		VFLOAT3(VFLOAT3&&) = default;
		VFLOAT3& operator=(VFLOAT3&&) = default;

		VFLOAT3(float _x, float _y, float _z) :x(_x), y(_y), z(_z) {}
		
		VFLOAT3 operator + (const VFLOAT3& rhs) const { return VFLOAT3(x + rhs.x, y + rhs.y, z + rhs.z); }
		VFLOAT3 operator - (const VFLOAT3& rhs) const { return VFLOAT3(x - rhs.x, y - rhs.y, z - rhs.z); }
		VFLOAT3 operator * (const VFLOAT3& rhs) const { return VFLOAT3(x * rhs.x, y * rhs.y, z * rhs.z); }
		VFLOAT3 operator * (float rhs) const { return VFLOAT3(x * rhs, y * rhs, z * rhs); }
		VFLOAT3 operator / (float rhs) const { return VFLOAT3(x / rhs, y / rhs, z / rhs); }
	};

	// Vec 4f
	struct VFLOAT4
	{
		float x;
		float y;
		float z;
		float w;
		VFLOAT4() = default;
		VFLOAT4(const VFLOAT4&) = default;
		VFLOAT4& operator=(const VFLOAT4&) = default;
		VFLOAT4(VFLOAT4&&) = default;
		VFLOAT4& operator=(VFLOAT4&&) = default;

		VFLOAT4(float _x, float _y, float _z, float _w) :x(_x), y(_y), z(_z), w(_w) {}
	};

	// Vec 2uint
	struct VUINT2
	{
		uint32_t x;
		uint32_t y;
		VUINT2() = default;
		VUINT2(const VUINT2&) = default;
		VUINT2& operator=(const VUINT2&) = default;
		VUINT2(VUINT2&&) = default;
		VUINT2& operator=(const VUINT2&) = default;

		VUINT2(uint32_t _x, uint32_t _y) :x(_x), y(_y) {}
	};

	// Matrix44f
	struct VMATRIX44
	{
		union
		{
			struct
			{
				float _11, _12, _13, _14;
				float _21, _22, _23, _24;
				float _31, _32, _33, _34;
				float _41, _42, _43, _44;
			};
			float m[4][4];
			float f[16];
		};
		VMATRIX44() = default;
		VMATRIX44(const VMATRIX44&) = default;
		VMATRIX44& operator=(const VMATRIX44&) = default;
		VMATRIX44(VMATRIX44&&) = default;
		VMATRIX44& operator=(const VMATRIX44&) = default;

		// Mat4 * Vec4 -> Vec4
		VFLOAT4 mulVec4(const VFLOAT4& a)
		{
			return VFLOAT4(
				_11 * a.x + _12 * a.y + _13 * a.z + _14 * a.w,
				_21 * a.x + _22 * a.y + _23 * a.z + _24 * a.w,
				_31 * a.x + _32 * a.y + _33 * a.z + _34 * a.w,
				_41 * a.x + _42 * a.y + _43 * a.z + _44 * a.w
			);
		}
		// Mat4 * [Vec3 1] -> Vec4
		VFLOAT4 mulVec3(const VFLOAT3& a)
		{
			return VFLOAT4(
				_11 * a.x + _12 * a.y + _13 * a.z + _14,
				_21 * a.x + _22 * a.y + _23 * a.z + _24,
				_31 * a.x + _32 * a.y + _33 * a.z + _34,
				_41 * a.x + _42 * a.y + _43 * a.z + _44
			);
		}
		// Mat4 * Vec3 -> Vec3 (Only direction component)
		VFLOAT3 mulDirVec3(const VFLOAT3& a)
		{
			return VFLOAT3(
				_11 * a.x + _12 * a.y + _13 * a.z,
				_21 * a.x + _22 * a.y + _23 * a.z,
				_31 * a.x + _32 * a.y + _33 * a.z
			);
		}
	};
	typedef auto_ptr<VMATRIX44> P_VMATRIX44;

	// TODO: Add quaternion support.

	/************************************************************************************
	Math operations
	************************************************************************************/
	VFLOAT2 operator * (float lhs, const VFLOAT2& rhs);

	VFLOAT3 operator * (float lhs, const VFLOAT3& rhs);
	
	// Matrix4 multiplication
	P_VMATRIX44 matMul(const P_VMATRIX44& lhs, const P_VMATRIX44& rhs);

	// Vec3f dot product
	float vecDot(const VFLOAT3& lhs, const VFLOAT3& rhs);

	// Vec3f cross product
	VFLOAT3 vecCross(const VFLOAT3& lhs, const VFLOAT3& rhs);

	// Vec3f normalize
	VFLOAT3 vecNormal(const VFLOAT3& a);

	/************************************************************************************
	Matrix functions
	************************************************************************************/
	/**
	Get right-handed perspective projection matrix
	w  - width of near-plane
	h  - height of near-plane
	zn - z value of near-plane
	zf - z value of far-plane
	**/
	P_VMATRIX44 getPerspectiveRH(float w, float h, float zn, float zf);

	// Get X-rotation matrix
	P_VMATRIX44 getRotationX(float);

	// Get Y-rotation matrix
	P_VMATRIX44 getRotationY(float);

	// Get Z-rotation matrix
	P_VMATRIX44 getRotationZ(float);

	/**
	Get right-handed look at matrix
	pEye - camera position
	pAt  - look at position
	pUp  - world up direction
	**/
	P_VMATRIX44 getLookAtRH(const VFLOAT3& pEye, const VFLOAT3& pAt, const VFLOAT3& pUp);

	// TODO: getLookAtFovRH()
}

#endif
