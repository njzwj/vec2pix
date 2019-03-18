#include <iostream>
#include <memory>
#include <stdint.h>
#include <vector>
#include <Windows.h>
#include "v2pbase.h"
#include "svpng.h"
using namespace std;
using namespace v2p;


int main()
{
	const uint16_t WIDTH = 512, HEIGHT = 512;
	const float PI = acosf(-1.0f);
	// test scheme
	vector<VERTEX> vBuf = {
		{{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},   // LT
		{{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},  // LB
		{{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},   // RB
		{{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},    // RT

		{{-1.0f, 1.0f, -1.0f}, {0.5f, 1.0f, 0.5f}},   // BLT
		{{-1.0f, -1.0f, -1.0f}, {1.0f, 0.5f, 0.5f}},  // BLB
		{{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}},   // BRB
		{{1.0f, 1.0f, -1.0f}, {0.5f, 0.5f, 1.0f}}     // BRT
	};
	vector<uint16_t> iBuf = { 
		0, 1, 2, 0, 2, 3,	// F
		3, 2, 6, 3, 6, 7,	// R
		4, 5, 1, 4, 1, 0,	// L
		7, 6, 5, 7, 5, 4,	// B
		4, 0, 3, 4, 3, 7,	// U
		1, 5, 6, 1, 6, 2	// D
	};
	vector<FRAGMENT> fBuf;

	for (uint32_t t = 0; t < 20; ++t)
	{
		float angle = t * PI / 10.0f, angle2 = PI/8.0f*sinf(PI/20.0*t);
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
		for (auto x : fBuf)
		{
			float z_hdc;

			uint8_t *p = scr + 3 * (x.uv.x + (HEIGHT - 1 - x.uv.y)*WIDTH);

			//*p++ = (uint8_t)roundf(x.z * 255);
			//*p++ = (uint8_t)roundf(x.z * 255);
			//*p++ = (uint8_t)roundf(x.z * 255);
			*p++ = (uint8_t)roundf(x.color.y * 255);
			*p++ = (uint8_t)roundf(x.color.z * 255);
			*p++ = (uint8_t)roundf(x.color.x * 255);
		}

		FILE* fp;
		char fn[25];
		sprintf_s((char* const)fn, 25, "output_%d.png", t);
		fopen_s(&fp, fn, "wb");
		svpng(fp, WIDTH, HEIGHT, (const unsigned char *)scr, 0);
		fclose(fp);
	}
}
