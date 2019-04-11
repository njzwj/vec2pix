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
	tex1 = new TEXTURE_BUFFER(64, 64);
	tex2 = new TEXTURE_BUFFER(64, 64);
	long c_white = 0xffffff, c_red = 0xff0000, c_blue = 0x0000ff;
	for (long i = 0; i < 64; ++i)
		for (long j = 0; j < 64; ++j)
		{
			long u = i / 8, v = j / 8;
			if ((u + v) & 1) tex1->SetPixel(j, i, c_white);
			else tex1->SetPixel(j, i, c_red);
			if ((u + v) & 1) tex2->SetPixel(j, i, c_blue);
			else tex2->SetPixel(j, i, c_white);
		}
}

VFLOAT3 BilinearTextureSampler(const TEXTURE_BUFFER *tex, float u, float v)
{
	static const float w = 1.0f / 256;
	long pos, ui, vi;
	float s[2], t[2], r = 0, g = 0, b = 0, fact;
	u = u * (tex->width - 1);
	v = v * (tex->height - 1);
	ui = floorf(u);
	vi = floorf(v);
	s[1] = u - ui;
	t[1] = v - vi;
	s[0] = 1.0f - s[1];
	t[0] = 1.0f - t[1];
	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			fact = s[j] * t[i];
			pos = 3 * ((ui + j) + (vi + i) * tex->width);
			r += fact * tex->buffer[pos];
			g += fact * tex->buffer[pos + 1];
			b += fact * tex->buffer[pos + 2];
		}
	}
	return VFLOAT3(r * w, g * w, b * w);
}

VFLOAT3 SimpleTextureSampler(const TEXTURE_BUFFER *tex, float u, float v)
{
	static const float w = 1.0f / 256;
	long pos;
	u = roundf(u * (tex->width - 1) + 0.49);
	v = roundf(v * (tex->height - 1) + 0.49);
	pos = (u + v * tex->width) * 3;
	return VFLOAT3(tex->buffer[pos] * w, tex->buffer[pos + 1] * w, tex->buffer[pos + 2] * w);
}

VFLOAT3 light;

void SimpleFragShader(DEVICE *device, VFLOAT3 pEye)
{
	// simple light
	const float shininess = 10.0;

	for (auto x : device->frag_buffer)
	{
		long z_buf_pos = x.uv.x + x.uv.y * device->width;
		if (device->z_buffer[z_buf_pos] < x.z) continue;
		device->z_buffer[z_buf_pos] = x.z;

		uint8_t *p = device->frame_buffer + 3 * (x.uv.x + (device->height - 1 - x.uv.y)*device->width);
		VFLOAT3 color;
		if (device->texture_filter_method == OFF)
			color = SimpleTextureSampler(device->texture, x.texCoord.x, x.texCoord.y);
		else
			color = BilinearTextureSampler(device->texture, x.texCoord.x, x.texCoord.y);

		// simple lighting : light from vp
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
	device->texture_filter_method = BILINEAR;

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

		light = vecNormal(VFLOAT3(-pEye.x, -pEye.y, -pEye.z));

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
	}
}
