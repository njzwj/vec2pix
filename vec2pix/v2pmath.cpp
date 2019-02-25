#include "v2pmath.h"
#include <math.h>

using namespace std;

namespace v2p
{
	VFLOAT2 operator * (float lhs, const VFLOAT2& rhs)
	{
		return VFLOAT2(lhs * rhs.x, lhs * rhs.y);
	}

	VFLOAT3 operator * (float lhs, const VFLOAT3& rhs)
	{
		return VFLOAT3(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z);
	}

	P_VMATRIX44 matMul(const P_VMATRIX44& lhs, const P_VMATRIX44& rhs)
	{
		P_VMATRIX44 pMat;
		for (uint16_t i = 0; i < 4; ++i)
			for (uint16_t j = 0; j < 4; ++j)
				pMat->m[i][j] =
				lhs->m[i][0] * rhs->m[0][j] +
				lhs->m[i][1] * rhs->m[1][j] +
				lhs->m[i][2] * rhs->m[2][j] +
				lhs->m[i][3] * rhs->m[3][j];
		return pMat;
	}

	float vecDot(const VFLOAT3& lhs, const VFLOAT3& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	}

	VFLOAT3 vecCross(const VFLOAT3& lhs, const VFLOAT3& rhs)
	{
		return VFLOAT3(
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x
		);
	}

	VFLOAT3 vecNormal(const VFLOAT3& a)
	{
		float len = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
		return VFLOAT3(a.x / len, a.y / len, a.z / len);
	}

	P_VMATRIX44 getPerspectiveRH(float w, float h, float zn, float zf)
	{
		return P_VMATRIX44(new VMATRIX44({
			2 * zn / w, 0.0f, 0.0f, 0.0f, 
			0.0f, 2 * zn / h, 0.0f, 0.0f,
			0.0f, 0.0f, zf/(zn-zf), zn*zf/(zn-zf),
			0.0f, 0.0f, -1.0f, 0.0f
			}
		));
	}

	P_VMATRIX44 getRotationX(float angle)
	{
		float c = cosf(angle), s = sinf(angle);
		return P_VMATRIX44(new VMATRIX44({
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f,    c,   -s, 0.0f,
			0.0f,    s,    c, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			}
		));
	}

	P_VMATRIX44 getRotationY(float angle)
	{
		float c = cosf(angle), s = sinf(angle);
		return P_VMATRIX44(new VMATRIX44({
			   c, 0.0f,    s, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			  -s, 0.0f,    c, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			}
		));
	}

	P_VMATRIX44 getRotationZ(float angle)
	{
		float c = cosf(angle), s = sinf(angle);
		return P_VMATRIX44(new VMATRIX44({
			   c,   -s, 0.0f, 0.0f,
			   s,    c, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			}
		));
	}

	P_VMATRIX44 getLookAtRH(const VFLOAT3& pEye, const VFLOAT3& pAt, const VFLOAT3& pUp)
	{
		VFLOAT3 zaxis = vecNormal(pEye - pAt);
		VFLOAT3 xaxis = vecNormal(vecCross(pUp, zaxis));
		VFLOAT3 yaxis = vecCross(zaxis, xaxis);
		return P_VMATRIX44(new VMATRIX44({
			xaxis.x, xaxis.y, xaxis.z, vecDot(xaxis, pEye),
			yaxis.x, yaxis.y, yaxis.z, vecDot(yaxis, pEye),
			zaxis.x, zaxis.y, zaxis.z, vecDot(zaxis, pEye),
			   0.0f,    0.0f,    0.0f, 1.0f
			}
		));
	}
}
