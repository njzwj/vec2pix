#include <iostream>
#include <memory>
#include <stdint.h>
#include <vector>
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
		{{-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}},   // LT
		{{-1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},  // LB
		{{1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},   // RB
		{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}}     // RT
	};
	vector<uint16_t> iBuf = { 0, 1, 2, 0, 2, 3 };
	vector<FRAGMENT> fBuf;

	for (uint32_t t = 0; t < 5; ++t)
	{
		float angle = 10.0f * t / 180.0f * PI;
		float d = 2.0f;
		VFLOAT3 pEye = { d * sinf(angle), 0.5f, d * cosf(angle) }, pAt = { 0.0f, 0.0f, 0.0f }, pUp = { 0.0f, 1.0f, 0.0f };
		P_VMATRIX44 w2c = getLookAtRH(pEye, pAt, pUp);
		P_VMATRIX44 proj = getPerspectiveRH(1.0f, 1.0f, 0.5f, 20.0f);

		fBuf.clear();
		rasterizeTriangleByIndex(vBuf, iBuf, fBuf, w2c, proj, WIDTH, HEIGHT);

		uint8_t scr[HEIGHT * WIDTH * 3];
		memset(scr, 0, sizeof(scr));
		for (auto x : fBuf)
		{
			uint8_t *p = scr + 3 * (x.uv.x + (HEIGHT - 1 - x.uv.y)*WIDTH);
			
			*p++ = (uint8_t)roundf(x.color.x * 255);
			*p++ = (uint8_t)roundf(x.color.y * 255);
			*p++ = (uint8_t)roundf(x.color.z * 255);
			//*p = (uint8_t)roundf(x.z * 255);
		}

		FILE* fp;
		char fn[25];
		sprintf_s((char* const)fn, 25, "output_%d.png", t);
		fopen_s(&fp, fn, "wb");
		svpng(fp, WIDTH, HEIGHT, (const unsigned char *)scr, 0);
		fclose(fp);
	}
}
