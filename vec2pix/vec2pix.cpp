#define _CRT_SECURE_NO_WARNINGS
// 设置绘制方法
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
#include "svpng.h"
#include <cassert>

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

// ======================================================
// Gamma Texture Fragmentshader ...
// ======================================================
//
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

TEXTURE_BUFFER* GetChessBoard(long w, long h, long inter, long c1, long c2)
{
	TEXTURE_BUFFER *tex = new TEXTURE_BUFFER(w, h);
	for (int i = 0; i < h; ++i)
	{
		for (int j = 0; j < w; ++j)
		{
			if ((i / inter + j / inter) & 1)
				tex->SetPixel(j, i, c1);
			else
				tex->SetPixel(j, i, c2);
		}
	}
	return tex;
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

VFLOAT3 BilinearTextureSampler(const TEXTURE_BUFFER *tex, float u, float v)
{
	if (tex->width == 1 || tex->height == 1)
		return SimpleTextureSampler(tex, u, v);
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

VFLOAT3 TrilinearTextureSampler(const TEXTURE_BUFFER_MIPMAP *tex, const FRAGMENT& frag)
{
	static const float w = 1.0f / 256;
	float l1, l2, l, u = frag.texCoord.x, v = frag.texCoord.y, z;
	long zi;
	VFLOAT3 c1, c2;
	const VFLOAT2 &duxy = frag.duxy, &dvxy = frag.dvxy;
	l1 = sqrtf(duxy.x * duxy.x + dvxy.x * dvxy.x);
	l2 = sqrtf(duxy.y * duxy.y + dvxy.y * dvxy.y);
	l = l1 > l2 ? l1 : l2;

	l = - log2f(l);
	zi = floorf(l);
	if (zi < 0) return SimpleTextureSampler(tex->buffer[tex->level - 1], 0.0f, 0.0f);
	if (zi >= tex->level - 1) return BilinearTextureSampler(tex->buffer[0], u, v);

	z = l - zi;
	c1 = BilinearTextureSampler(tex->buffer[tex->level - 1 - zi], u, v);
	c2 = BilinearTextureSampler(tex->buffer[tex->level - 2 - zi], u, v);
	return z * c2 + (1.0f - z) * c1;
}

void SimpleFragShader(DEVICE *device, VFLOAT3 pEye)
{
	// simple light
	const float shininess = 1.0;
	VFLOAT3 c_tex, c, l_dir, l_h, view_dir;
	VFLOAT3 c_ambient, c_diffuse, c_specular;
	VFLOAT3 l_ambient, l_diffuse, l_specular;
	l_ambient = device->light->ambient;
	l_diffuse = device->light->diffuse;
	l_specular = device->light->specular;

	for (auto x : device->frag_buffer)
	{
		long z_buf_pos = x.uv.x + x.uv.y * device->width;
		if (device->z_buffer[z_buf_pos] < x.z) continue;
		device->z_buffer[z_buf_pos] = x.z;

		uint8_t *p = device->frame_buffer + 3 * (x.uv.x + (device->height - 1 - x.uv.y)*device->width);
		
		// texture sampler
		if (device->texture)
		{
			if (device->texture_filter_method == OFF)
				c_tex = SimpleTextureSampler((TEXTURE_BUFFER*)device->texture, x.texCoord.x, x.texCoord.y);
			else if (device->texture_filter_method == BILINEAR)
				c_tex = BilinearTextureSampler((TEXTURE_BUFFER*)device->texture, x.texCoord.x, x.texCoord.y);
			else if (device->texture_filter_method == TRILINEAR)
				c_tex = TrilinearTextureSampler((TEXTURE_BUFFER_MIPMAP*)device->texture, x);
		}

		// light & material
		if (device->material->type | DIFFUSE_MAP)
		{
			c_ambient = c_tex;
			c_diffuse = c_tex;
			c_specular = device->material->specular;
		}
		else 
		{
			c_ambient = device->material->ambient;
			c_diffuse = device->material->diffuse;
			c_specular = device->material->specular;
		}

		if (device->material->type | BLINNPHONG)
		{
			float spec, diff;
			view_dir = vecNormal(pEye - x.position);
			l_dir = vecNormal(device->light->position - x.position);
			l_h = vecNormal(view_dir + l_dir);
			diff = max(vecDot(l_dir, x.normal), 0.0f);
			spec = powf(max(vecDot(l_h, x.normal), 0.0f), device->material->shininess);

			c_ambient = c_ambient * l_ambient;
			c_diffuse = c_diffuse * l_diffuse * diff;
			c_specular = c_specular * l_specular * spec;
			
			c = c_ambient + c_diffuse + c_specular;
			c.clip(0.0f, 1.0f);
		}

		*p++ = (uint8_t)(gamma_to_linear[(int)roundf(c.z * 255)] / 256);
		*p++ = (uint8_t)(gamma_to_linear[(int)roundf(c.y * 255)] / 256);
		*p++ = (uint8_t)(gamma_to_linear[(int)roundf(c.x * 255)] / 256);

		++frag_cnt;
	}
	device->frag_buffer.clear();
}

// ======================================================
// Render state management
// ======================================================
//
DEVICE *device = nullptr;

void SetupDevice(long width, long height)
{
	device = new DEVICE(width, height);
}

void ClearDevice()
{
	device->texture = nullptr; 
	device->material = nullptr;
	device->light = nullptr;
	
	device->frag_buffer.clear();
	memset(device->frame_buffer, 0, device->width * device->height * 3);
	for (int i = 0; i < device->width * device->height; ++i)
		device->z_buffer[i] = 1.0f;
}

void AnimeDemo()
{
	uint8_t *scr = NULL;
#ifdef DRAW_IN_GDI
	const long WIDTH = 512, HEIGHT = 512;
#endif
#ifdef DRAW_IN_CONSOLE
	const uint16_t WIDTH = 64, HEIGHT = 32;
	HANDLE h_con = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
	const float PI = acosf(-1.0f);
	// init device
	SetupDevice(WIDTH, HEIGHT);
	device->texture_filter_method = TRILINEAR;

#ifdef DRAW_IN_GDI
	InitWindow(WIDTH, HEIGHT, L"V2p demo", &scr);
	delete[] device->frame_buffer;
	device->frame_buffer = scr;
#endif

#ifdef DRAW_IN_CONSOLE
	InitDrawBuffer(h_con, WIDTH, HEIGHT);
#endif

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

	MATERIAL mat = { (MATERIAL_TYPE)(BLINNPHONG | DIFFUSE_MAP), {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 5.0f };
	LIGHT light = { POINTLIGHT, {0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 0.0f}, {0.5f, 0.45f, 0.45f}, {0.5f, 0.45f, 0.45f} };
	TEXTURE_BUFFER *tex1 = nullptr, *tex2 = nullptr;
	TEXTURE_BUFFER_MIPMAP *tex1_mipmap = nullptr, *tex2_mipmap = nullptr;
	long w = 256, h = 256;
	long c_white = 0xffffff, c_blue = 0xff0000, c_red = 0x0000ff;
	tex1 = GetChessBoard(w, h, 8, c_white, c_red);
	tex2 = GetChessBoard(w, h, 8, c_blue, c_white);
	tex1_mipmap = new TEXTURE_BUFFER_MIPMAP(tex1);
	tex2_mipmap = new TEXTURE_BUFFER_MIPMAP(tex2);

	for (int t = 0; ; ++t)
	{
		timer_st = clock();

		float angle = PI * ((t + 0.5f) / 60.0f) - PI / 1.5, angle2 = PI / 8.0f * sinf(PI / 60.0*t);
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

		device->material = &mat;
		device->light = &light;

		device->texture = tex1_mipmap;
		RasterizeTriangleByIndex(device, vBuf, iBuf1, w2c, proj);
		SimpleFragShader(device, pEye);
		device->texture = tex2_mipmap;
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
		if (t >= 70) t = 0;
	}
}

void TextureFilterDemo()
{
	const long WIDTH = 512, HEIGHT = 512;
	const float PI = acosf(-1.0f);
	char fn[256];
	FILE *fp;
	TEXTURE_BUFFER *tex = GetChessBoard(1024, 1024, 21, 0xffffff, 0x000000);
	TEXTURE_BUFFER_MIPMAP *tex_mip = new TEXTURE_BUFFER_MIPMAP(tex);

	SetupDevice(WIDTH, HEIGHT);

	vector<VERTEX> v_buf = {
		{ {-10.0f, 10.0f, 0.0f}, {0.5f, 0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, // LT
		{ {-10.0f,-10.0f, 0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // LB
		{ { 10.0f,-10.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // RB
		{ { 10.0f, 10.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }  // RT
	};
	vector<uint16_t> i_buf = {
		0, 1, 3, 1, 2, 3
	};
	VFLOAT3 pEye = { 0.0f, -10.0f, 1.0f }, pAt = { 0.0f, -7.5f, 0.0f }, pUp = { 0.0f, 0.0f, 1.0f };
	P_VMATRIX44 w2c = getLookAtRH(pEye, pAt, pUp);
	P_VMATRIX44 proj = getPerspectiveRH(0.2f, 0.1f, 0.1f, 20.0f);
	MATERIAL mat = { (MATERIAL_TYPE)(BLINNPHONG | DIFFUSE_MAP), {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 5.0f };
	LIGHT light = { POINTLIGHT, {0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f} };
	device->material = &mat;
	device->light = &light;

	// ========= trilinear ==========
	ClearDevice();
	device->texture_filter_method = TRILINEAR;
	memset(device->frame_buffer, 64, device->width * device->height * 3);
	frag_cnt = 0;
	device->texture = tex_mip;
	device->material = &mat;
	device->light = &light;
	RasterizeTriangleByIndex(device, v_buf, i_buf, w2c, proj);
	SimpleFragShader(device, pEye);

	sprintf(fn, "texfilter_trilinear.png");
	fp = fopen(fn, "wb");
	svpng(fp, device->width, device->height, device->frame_buffer, 0);

	// ========= bilinear ==========
	ClearDevice();
	device->texture_filter_method = BILINEAR;
	memset(device->frame_buffer, 64, device->width * device->height * 3);
	frag_cnt = 0;
	device->texture = tex;
	device->material = &mat;
	device->light = &light;
	RasterizeTriangleByIndex(device, v_buf, i_buf, w2c, proj);
	SimpleFragShader(device, pEye);

	sprintf(fn, "texfilter_bilinear.png");
	fp = fopen(fn, "wb");
	svpng(fp, device->width, device->height, device->frame_buffer, 0);

	// ========= nearest ==========
	ClearDevice();
	device->texture_filter_method = OFF;
	memset(device->frame_buffer, 64, device->width * device->height * 3);
	frag_cnt = 0;
	device->texture = tex;
	device->material = &mat;
	device->light = &light;
	RasterizeTriangleByIndex(device, v_buf, i_buf, w2c, proj);
	SimpleFragShader(device, pEye);

	sprintf(fn, "texfilter_nearest.png");
	fp = fopen(fn, "wb");
	svpng(fp, device->width, device->height, device->frame_buffer, 0);
}

int main()
{
	GenerateGamma();
	SetConsoleTitle(L"V2p demo");
	AnimeDemo();
	// TextureFilterDemo();
}
