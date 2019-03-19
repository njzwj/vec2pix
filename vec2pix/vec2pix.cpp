#include <iostream>
#include <memory>
#include <stdint.h>
#include <vector>
#include <Windows.h>
#include "v2pbase.h"
#include "svpng.h"
using namespace std;
using namespace v2p;


VFLOAT3 chessboardSampler(float u, float v, uint32_t row, uint32_t col)
{
	uint32_t a = (int)(u * col), b = (int)(v * row);
	if ((a ^ b) & 1) {
		return VFLOAT3(0.2f, 0.5f, 1.0f);
	}
	else 
	{
		return VFLOAT3(0.9f, 0.9f, 0.9f);
	}
}


int main()
{
	const uint16_t WIDTH = 512, HEIGHT = 512;
	const float PI = acosf(-1.0f);
	// test scheme
	vector<VERTEX> vBuf = {
		{ {-1.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, // LT
		{ {-1.0f,-1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // LB
		{ { 1.0f,-1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // RB
		{ { 1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }  // RT
	};
	vector<uint16_t> iBuf = {
		0, 1, 3, 1, 2, 3
	};
	vector<FRAGMENT> fBuf;

	for (uint32_t t = 0; t < 10; ++t)
	{
		float angle = PI * ((t + 2.5f) / 15.0f) - PI/2, angle2 = PI/8.0f*sinf(PI/20.0*t);
		float d = 5.0f;
		VFLOAT3 pEye = { d * sinf(angle) * cosf(angle2), d * sinf(angle2), d * cosf(angle) * cosf(angle2) }, 
		 	pAt = { 0.0f, 0.0f, 0.0f }, pUp = { 0.0f, 1.0f, 0.0f };
		P_VMATRIX44 w2c = getLookAtRH(pEye, pAt, pUp);
		P_VMATRIX44 proj = getPerspectiveRH(1.0f, 1.0f, 1.0f, 20.0f);

		DWORD tim = GetTickCount();
		fBuf.clear();
		rasterizeTriangleByIndex(vBuf, iBuf, fBuf, w2c, proj, WIDTH, HEIGHT);
		tim = GetTickCount() - tim;

		cout << "frame " << t << ", " << tim << "ms\n";

		uint8_t scr[HEIGHT * WIDTH * 3];
		memset(scr, 0, sizeof(scr));
		for (uint32_t i = 0; i < HEIGHT; ++i)
			for (uint32_t j = 0; j < WIDTH; ++j)
			{
				uint32_t pos = 3 * (i * WIDTH + j);
				scr[pos] = scr[pos + 1] = scr[pos + 2] = (int)(50.0f / HEIGHT * i + 90);
			}
		for (auto x : fBuf)
		{
			float z_hdc;

			uint8_t *p = scr + 3 * (x.uv.x + (HEIGHT - 1 - x.uv.y)*WIDTH);
			VFLOAT3 color = chessboardSampler(x.texCoord.x, x.texCoord.y, 5, 5);
			*p++ = (uint8_t)roundf(color.x * 255);
			*p++ = (uint8_t)roundf(color.y * 255);
			*p++ = (uint8_t)roundf(color.z * 255);
		}

		FILE* fp;
		char fn[25];
		sprintf_s((char* const)fn, 25, "output_%d.png", t);
		fopen_s(&fp, fn, "wb");
		svpng(fp, WIDTH, HEIGHT, (const unsigned char *)scr, 0);
		fclose(fp);
	}
}
