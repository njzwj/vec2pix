#include <iostream>
#include <memory>
#include <stdint.h>
#include <vector>
#include "v2pbase.h"
using namespace std;
using namespace v2p;


int main()
{
	const uint16_t WIDTH = 50, HEIGHT = 25;
	const float PI = acosf(-1.0f);
	// test scheme
	vector<VERTEX> vBuf = {
		{{-1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},   // LT
		{{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // LB
		{{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},   // RB
		{{1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}     // RT
	};
	vector<uint16_t> iBuf = { 0, 1, 2, 2, 3, 0 };
	vector<FRAGMENT> fBuf;

	for (uint32_t t = 0; t < 9; ++t)
	{
		float angle = 10.0f * t / 180.0f * PI;
		float d = 2.0f;
		VFLOAT3 pEye = { d * sinf(angle), 1.0f, d * cosf(angle) }, pAt = { 0.0f, 0.0f, 0.0f }, pUp = { 0.0f, 1.0f, 0.0f };
		P_VMATRIX44 w2c = getLookAtRH(pEye, pAt, pUp);
		P_VMATRIX44 proj = getPerspectiveRH(1.0f, 1.0f, 0.5f, 20.0f);

		fBuf.clear();
		rasterizeTriangleByIndex(vBuf, iBuf, fBuf, w2c, proj, WIDTH, HEIGHT);

		uint8_t scr[HEIGHT][WIDTH] = { '-' };
		memset(scr, '-', sizeof(scr));
		for (auto x : fBuf)
		{
			scr[HEIGHT - 1 - x.uv.y][x.uv.x] = '#';
		}
		for (uint32_t i = 0; i < HEIGHT; ++i)
		{
			for (uint32_t j = 0; j < WIDTH; ++j)
				cout << scr[i][j];
			cout << '\n';
		}
		cout << "================\n";
	}
}
