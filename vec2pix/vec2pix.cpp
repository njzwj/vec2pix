// #define DRAW_IN_CONSOLE
#define DRAW_IN_GDI

#include <memory>
#include <stdint.h>
#include <vector>
#include <Windows.h>
#include <stdio.h>
#include <ctime>
#include <algorithm>
#include "v2pbase.h"

#ifdef DRAW_IN_GDI
#include "drawgdi.h"
#endif

#ifdef DRAW_IN_CONSOLE
#include "drawcon.h"
#endif

using namespace std;
using namespace v2p;

// monitor
unsigned int frag_cnt;
clock_t timer_st, timer_ed;

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

// render state management
TEXTURE_BUFFER *tex1 = nullptr, *tex2 = nullptr;
DEVICE *device = nullptr;

void SetupDevice(long width, long height)
{
	device = new DEVICE(width, height);
}

void ClearDevice()
{
	device->texture = nullptr;
	device->frag_buffer.clear();
	memset(device->frame_buffer, 0, device->width * device->height * 3);
	for (int i = 0; i < device->width * device->height; ++i)
		device->z_buffer[i] = 1.0f;
}

void SetupTextures()
{
	tex1 = new TEXTURE_BUFFER(128, 128);
	tex2 = new TEXTURE_BUFFER(128, 128);
	long c_white = 0xffffff, c_red = 0xff0000, c_blue = 0x0000ff;
	for (long i = 0; i < 128; ++i)
		for (long j = 0; j < 128; ++j)
		{
			long u = i >> 4, v = j >> 4;
			if ((u + v) & 1) tex1->SetPixel(j, i, c_white);
			else tex1->SetPixel(j, i, c_red);
			if ((u + v) & 1) tex2->SetPixel(j, i, c_blue);
			else tex2->SetPixel(j, i, c_white);
		}
}

VFLOAT3 SimpleTextureSampler(const TEXTURE_BUFFER *tex, float u, float v)
{
	static const float w = 1.0f / 256;
	long pos;
	u = floorf(u * tex->width);
	v = floorf(v * tex->height);
	pos = (u + v * tex->width) * 3;
	return VFLOAT3(tex->buffer[pos] * w, tex->buffer[pos + 1] * w, tex->buffer[pos + 2] * w);
}

void SimpleFragShader(DEVICE *device, VFLOAT3 pEye)
{
	// simple light
	VFLOAT3 light = { 0.0f, 0.0f, -1.0f };
	const float shininess = 10.0;

	for (auto x : device->frag_buffer)
	{
		// if (z_buf.getBuffer(x.uv.x, x.uv.y) < x.z) continue;
		// z_buf.setBuffer(x.uv.x, x.uv.y, x.z);
		long z_buf_pos = x.uv.x + x.uv.y * device->width;
		if (device->z_buffer[z_buf_pos] < x.z) continue;
		device->z_buffer[z_buf_pos] = x.z;

		uint8_t *p = device->frame_buffer + 3 * (x.uv.x + (device->height - 1 - x.uv.y)*device->width);
		VFLOAT3 color = SimpleTextureSampler(device->texture, x.texCoord.x, x.texCoord.y);

		// simple lighting : light from z+
		// BlinnPhong
		VFLOAT3 v = vecNormal(pEye - x.position), h = vecNormal(v - light);
		float spec = powf(max(vecDot(h, x.normal), 0.0f), shininess);
		color = color * (spec + 0.5f) / 1.5f;

		*p++ = (uint8_t)(gamma_to_linear[(int)roundf(color.x * 255)] / 256);
		*p++ = (uint8_t)(gamma_to_linear[(int)roundf(color.y * 255)] / 256);
		*p++ = (uint8_t)(gamma_to_linear[(int)roundf(color.z * 255)] / 256);

		++frag_cnt;
	}
	device->frag_buffer.clear();
}


int main()
{
#ifdef DRAW_IN_GDI
	const long WIDTH = 256, HEIGHT = 256;
#endif
#ifdef DRAW_IN_CONSOLE
	const uint16_t WIDTH = 96, HEIGHT = 48;
	HANDLE h_con = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
	const float PI = acosf(-1.0f);
	uint8_t *scr = NULL;

	GenerateGamma();

	SetConsoleTitle(L"V2p demo");

	// init device
	SetupDevice(WIDTH, HEIGHT);

#ifdef DRAW_IN_GDI
	InitWindow(WIDTH, HEIGHT, L"V2p demo", &scr);
	delete[] device->frame_buffer;
	device->frame_buffer = scr;
#endif

#ifdef DRAW_IN_CONSOLE
	InitDrawBuffer(h_con, WIDTH, HEIGHT);
#endif

	SetupTextures();

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

	for (int t = -15; ; ++t)
	{
		timer_st = clock();

		float angle = PI * ((t + 0.5f) / 60.0f) - PI/2, angle2 = PI / 8.0f * sinf(PI/60.0*t);
		float d = 5.0f;
		VFLOAT3 pEye = { d * sinf(angle) * cosf(angle2), d * sinf(angle2), d * cosf(angle) * cosf(angle2) }, 
		 	pAt = { 0.0f, 0.0f, 0.0f }, pUp = { 0.0f, 1.0f, 0.0f };
		P_VMATRIX44 w2c = getLookAtRH(pEye, pAt, pUp);
		P_VMATRIX44 proj = getPerspectiveRH(1.0f, 1.0f, 1.0f, 20.0f);

#ifdef DRAW_IN_GDI
		DispatchEvent();
#endif
		ClearDevice();
		memset(device->frame_buffer, 128, device->width * device->height * 3);

		frag_cnt = 0;
		device->texture = tex1;
		RasterizeTriangleByIndex(device, vBuf, iBuf1, w2c, proj);
		SimpleFragShader(device, pEye);
		device->texture = tex2;
		RasterizeTriangleByIndex(device, vBuf, iBuf2, w2c, proj);
		SimpleFragShader(device, pEye);

		timer_ed = clock();
#ifdef DRAW_IN_CONSOLE
		DrawCon(device->frame_buffer);
#endif
#ifdef DRAW_IN_GDI
		UpdateScreen();
		printf("Frag %u, %d ms\n", frag_cnt, (long)(((double)timer_ed - timer_st) / CLOCKS_PER_SEC * 1000));
		if (exit_status) break;
#endif
		Sleep(10);
		if (t > 60)
			t = -15;
		
		// FILE* fp;
		// char fn[25];
		// sprintf_s((char* const)fn, 25, "output_%d.png", t);
		// fopen_s(&fp, fn, "wb");
		// svpng(fp, WIDTH, HEIGHT, (const unsigned char *)scr, 0);
		// fclose(fp);
	}
}
