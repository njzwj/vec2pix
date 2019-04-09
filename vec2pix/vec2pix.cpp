#include <iostream>
#include <memory>
#include <stdint.h>
#include <vector>
#include <Windows.h>
#include <stdio.h>
#include <algorithm>
#include "v2pbase.h"
#include "svpng.h"
#include "drawcon.h"
using namespace std;
using namespace v2p;

// gamma correction
const float GAMMA = 2.2;
unsigned short gamma_to_linear[256];
unsigned char linear_to_gamma[65536];

void GenerateGamma()
{
	int result;
	for (int i = 0; i < 256; i++) {
		result = (int)(pow(i / 255.0, GAMMA)*65535.0 + 0.5);
		gamma_to_linear[i] = (unsigned short)result;
	}

	for (int i = 0; i < 65536; i++) {
		result = (int)(pow(i / 65535.0, 1 / GAMMA)*255.0 + 0.5);
		linear_to_gamma[i] = (unsigned char)result;
	}
}


VFLOAT3 ChessboardSampler(float u, float v, uint32_t row, uint32_t col, VFLOAT3 c1, VFLOAT3 c2)
{
	uint32_t a = (int)(u * col), b = (int)(v * row);
	if ((a ^ b) & 1)
		return c1;
	else 
		return c2;
}

void SimpleFragShader(vector<FRAGMENT>& frag_buffer, uint8_t *scr, BUFFER2D<float>& z_buf, VFLOAT3 pEye,
	uint16_t WIDTH, uint16_t HEIGHT, VFLOAT3 c1, VFLOAT3 c2)
{
	// simple light
	VFLOAT3 light = { 0.0f, 0.0f, -1.0f };
	const float shininess = 10.0;

	for (auto x : frag_buffer)
	{
		if (z_buf.getBuffer(x.uv.x, x.uv.y) < x.z) continue;
		z_buf.setBuffer(x.uv.x, x.uv.y, x.z);

		uint8_t *p = scr + 3 * (x.uv.x + (HEIGHT - 1 - x.uv.y)*WIDTH);
		VFLOAT3 color = ChessboardSampler(x.texCoord.x, x.texCoord.y, 5, 5, c1, c2);

		// simple lighting : light from z+
		// BlinnPhong
		VFLOAT3 v = vecNormal(pEye - x.position), h = vecNormal(v - light);
		float spec = powf(max(vecDot(h, x.normal), 0.0f), shininess);
		color = color * (spec + 0.5f) / 1.5f;

		
		*p++ = (uint8_t)(gamma_to_linear[(int)roundf(color.x * 255)] / 256);
		*p++ = (uint8_t)(gamma_to_linear[(int)roundf(color.y * 255)] / 256);
		*p++ = (uint8_t)(gamma_to_linear[(int)roundf(color.z * 255)] / 256);
		/*
		*p++ = (uint8_t)roundf(color.x * 255);
		*p++ = (uint8_t)roundf(color.y * 255);
		*p++ = (uint8_t)roundf(color.z * 255);
		*/
	}
	frag_buffer.clear();
}

int main()
{
	const float PI = acosf(-1.0f);
	const uint16_t WIDTH = 64, HEIGHT = 32;
	TCHAR newTitle[] = TEXT("Vec2pix demo");
	HANDLE h_con = GetStdHandle(STD_OUTPUT_HANDLE);

	int unicodeShade[] = { 0x2591, 0x2592, 0x2593, 0x2588, 0x2592, 0x2593, 0x2588, 0x2588 };

	GenerateGamma();

	SetConsoleTitle(newTitle);

	InitDrawBuffer(h_con, WIDTH, HEIGHT);

	// test scheme
	vector<VERTEX> vBuf = {
		{ {-1.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, // LT
		{ {-1.0f,-1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // LB
		{ { 1.0f,-1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // RB
		{ { 1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, // RT

		{ {-1.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, // LT
		{ {-1.0f,-1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // LB
		{ { 1.0f,-1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // RB
		{ { 1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }  // RT
	};
	P_VMATRIX44 r1 = getRotationZ(PI / 8), r2 = getRotationY(-PI / 5), r3 = matMul(r1, matMul(r2, r1));
	for (uint32_t i = 4; i < 8; ++i) {
		VERTEX& x = vBuf[i];
		VFLOAT4 npos = r3->mulVec3(x.position);
		VFLOAT3 nnorm = r3->mulDirVec3(x.normal);
		x.position = { npos.x / npos.w, npos.y / npos.w, npos.z / npos.w };
		x.normal = nnorm;
	}
	vector<uint16_t> iBuf1 = {
		0, 1, 3, 1, 2, 3
	};
	vector<uint16_t> iBuf2 = {
		4, 5, 7, 5, 6, 7
	};

	vector<FRAGMENT> fBuf;

	for (int t = -15; ; ++t)
	{
		float angle = PI * ((t + 0.5f) / 60.0f) - PI/2, angle2 = PI / 8.0f * sinf(PI/60.0*t);
		float d = 5.0f;
		VFLOAT3 pEye = { d * sinf(angle) * cosf(angle2), d * sinf(angle2), d * cosf(angle) * cosf(angle2) }, 
		 	pAt = { 0.0f, 0.0f, 0.0f }, pUp = { 0.0f, 1.0f, 0.0f };
		P_VMATRIX44 w2c = getLookAtRH(pEye, pAt, pUp);
		P_VMATRIX44 proj = getPerspectiveRH(1.0f, 1.0f, 1.0f, 20.0f);

		uint8_t scr[HEIGHT * WIDTH * 3];
		// fill background
		memset(scr, 0, sizeof(scr));
		/*
		// transition background
		for (uint32_t i = 0; i < HEIGHT; ++i)
			for (uint32_t j = 0; j < WIDTH; ++j)
			{
				uint32_t pos = 3 * (i * WIDTH + j);
				scr[pos] = scr[pos + 1] = scr[pos + 2] = gamma_to_linear[(int)(50.0f / HEIGHT * i + 150)] / 256;
			}
		*/

		VFLOAT3 c1 = { 0.2f, 0.5f, 1.0f }, c2 = { 0.9f, 0.9f, 0.9f }, c3 = { 1.0f, 0.5f, 0.2f };
		// clear z-buffer
		BUFFER2D<float> z_buf = BUFFER2D<float>(WIDTH, HEIGHT);
		z_buf.fill(1.0f);

		RasterizeTriangleByIndex(vBuf, iBuf1, fBuf, w2c, proj, WIDTH, HEIGHT);
		SimpleFragShader(fBuf, scr, z_buf, pEye, WIDTH, HEIGHT, c1, c2);
		RasterizeTriangleByIndex(vBuf, iBuf2, fBuf, w2c, proj, WIDTH, HEIGHT);
		SimpleFragShader(fBuf, scr, z_buf, pEye, WIDTH, HEIGHT, c2, c3);

		DrawCon(scr);

		Sleep(50);
		if (t > 60)
			t = -15;

		/*
		FILE* fp;
		char fn[25];
		sprintf_s((char* const)fn, 25, "output_%d.png", t);
		fopen_s(&fp, fn, "wb");
		svpng(fp, WIDTH, HEIGHT, (const unsigned char *)scr, 0);
		fclose(fp);
		*/
	}
}
