#include <algorithm>
#include <cstdint>
#include <cmath>

/*

*******  Super XBR Scaler  *******

Copyright (c) 2016 Hyllian - sergiogdb@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#define USE_SQUARE 0

static inline __attribute((always_inline))
float R(uint32_t _col)
{
	int r = (_col >> 16) & 0xFF;
#if USE_SQUARE
	return r*r;
#else
	return r;
#endif
}

static inline __attribute((always_inline))
float G(uint32_t _col)
{
	int r = (_col >> 8) & 0xFF;
#if USE_SQUARE
	return r*r;
#else
	return r;
#endif
}

static inline __attribute((always_inline))
float B(uint32_t _col)
{
	int r = (_col) & 0xFF;
#if USE_SQUARE
	return r*r;
#else
	return r;
#endif
}

static inline __attribute((always_inline))
float A(uint32_t _col)
{
	int r = (_col >> 24) & 0xFF;
#if USE_SQUARE
	return r*r;
#else
	return r;
#endif
}

static constexpr float wgt1 = 0.129633f;
static constexpr float wgt2 = 0.175068f;
static constexpr float w1  = (-wgt1);
static constexpr float w2  = (wgt1+0.5f);
static constexpr float w3  = (-wgt2);
static constexpr float w4  = (wgt2+0.5f);

static
float getLuminescence(float r, float g, float b)
{
	return 0.2126*r + 0.7152*g + 0.0722*b;
}

float df(float A, float B)
{
	return abs(A - B);
}

float min4(float a, float b, float c, float d)
{
	return std::min(std::min(a,b),std::min(c, d));
}

float max4(float a, float b, float c, float d)
{
	return std::max(std::max(a, b), std::max(c, d));
}

template<class T>
T clamp(T x, T floor, T ceil)
{
	return std::max(std::min(x, ceil), floor);
}

static inline __attribute((always_inline))
uint32_t toColor(
	const float min_r, const float max_r, float rf,
	const float min_g, const float max_g, float gf,
	const float min_b, const float max_b, float bf,
	const float min_a, const float max_a, float af)
{
	rf = clamp(rf, min_r, max_r);
	gf = clamp(gf, min_g, max_g);
	bf = clamp(bf, min_b, max_b);
	af = clamp(af, min_a, max_a);

#if USE_SQUARE
	uint8_t ri = clamp((int) (sqrt(rf) + .5), 0, 255);
	uint8_t gi = clamp((int) (sqrt(gf) + .5), 0, 255);
	uint8_t bi = clamp((int) (sqrt(bf) + .5), 0, 255);
	uint8_t ai = clamp((int) (sqrt(af) + .5), 0, 255);
#else
	uint8_t ri = clamp((int) (rf + .5), 0, 255);
	uint8_t gi = clamp((int) (gf + .5), 0, 255);
	uint8_t bi = clamp((int) (bf + .5), 0, 255);
	uint8_t ai = clamp((int) (af + .5), 0, 255);

#endif
	return ((int) ai << 24) | ((int) ri << 16) | ((int) gi << 8) | bi;
}

static inline __attribute((always_inline))
uint32_t toColor(
	const float r[4][4], const float rf,
	const float g[4][4], const float gf,
	const float b[4][4], const float bf,
	const float a[4][4], const float af)
{
	float min_r_sample = min4(r[1][1], r[2][1], r[1][2], r[2][2]);
	float min_g_sample = min4(g[1][1], g[2][1], g[1][2], g[2][2]);
	float min_b_sample = min4(b[1][1], b[2][1], b[1][2], b[2][2]);
	float min_a_sample = min4(a[1][1], a[2][1], a[1][2], a[2][2]);
	float max_r_sample = max4(r[1][1], r[2][1], r[1][2], r[2][2]);
	float max_g_sample = max4(g[1][1], g[2][1], g[1][2], g[2][2]);
	float max_b_sample = max4(b[1][1], b[2][1], b[1][2], b[2][2]);
	float max_a_sample = max4(a[1][1], a[2][1], a[1][2], a[2][2]);

	return toColor(min_r_sample, max_r_sample, rf, min_g_sample, max_g_sample, gf, min_b_sample, max_b_sample, bf, min_a_sample, max_a_sample, af);
}


/*
                         P1
|P0|B |C |P1|         C     F4          |a0|b1|c2|d3|
|D |E |F |F4|      B     F     I4       |b0|c1|d2|e3|   |e1|i1|i2|e2|
|G |H |I |I4|   P0    E  A  I     P3    |c0|d1|e2|f3|   |e3|i3|i4|e4|
|P2|H5|I5|P3|      D     H     I5       |d0|e1|f2|g3|
                      G     H5
                         P2

sx, sy
-1  -1 | -2  0   (x+y) (x-y)    -3  1  (x+y-1)  (x-y+1)
-1   0 | -1 -1                  -2  0
-1   1 |  0 -2                  -1 -1
-1   2 |  1 -3                   0 -2

 0  -1 | -1  1   (x+y) (x-y)      ...     ...     ...
 0   0 |  0  0
 0   1 |  1 -1
 0   2 |  2 -2

 1  -1 |  0  2   ...
 1   0 |  1  1
 1   1 |  2  0
 1   2 |  3 -1

 2  -1 |  1  3   ...
 2   0 |  2  2
 2   1 |  3  1
 2   2 |  4  0


*/

float diagonal_edge(float mat[][4], float *wp) {
	float dw1 = wp[0]*(df(mat[0][2], mat[1][1]) + df(mat[1][1], mat[2][0]) + df(mat[1][3], mat[2][2]) + df(mat[2][2], mat[3][1])) +\
				wp[1]*(df(mat[0][3], mat[1][2]) + df(mat[2][1], mat[3][0])) + \
				wp[2]*(df(mat[0][3], mat[2][1]) + df(mat[1][2], mat[3][0])) +\
				wp[3]*df(mat[1][2], mat[2][1]) +\
				wp[4]*(df(mat[0][2], mat[2][0]) + df(mat[1][3], mat[3][1])) +\
				wp[5]*(df(mat[0][1], mat[1][0]) + df(mat[2][3], mat[3][2]));

	float dw2 = wp[0]*(df(mat[0][1], mat[1][2]) + df(mat[1][2], mat[2][3]) + df(mat[1][0], mat[2][1]) + df(mat[2][1], mat[3][2])) +\
				wp[1]*(df(mat[0][0], mat[1][1]) + df(mat[2][2], mat[3][3])) +\
				wp[2]*(df(mat[0][0], mat[2][2]) + df(mat[1][1], mat[3][3])) +\
				wp[3]*df(mat[1][1], mat[2][2]) +\
				wp[4]*(df(mat[1][0], mat[3][2]) + df(mat[0][1], mat[2][3])) +\
				wp[5]*(df(mat[0][2], mat[1][3]) + df(mat[2][0], mat[3][1]));

	return (dw1 - dw2);
}

// Not used yet...
float cross_edge(float mat[][4], float *wp) {
	float hvw1 = wp[3] * (df(mat[1][1], mat[2][1]) + df(mat[1][2], mat[2][2])) + \
				 wp[0] * (df(mat[0][1], mat[1][1]) + df(mat[2][1], mat[3][1]) + df(mat[0][2], mat[1][2]) + df(mat[2][2], mat[3][2])) + \
				 wp[2] * (df(mat[0][1], mat[2][1]) + df(mat[1][1], mat[3][1]) + df(mat[0][2], mat[2][2]) + df(mat[1][2], mat[3][2]));

	float hvw2 = wp[3] * (df(mat[1][1], mat[1][2]) + df(mat[2][1], mat[2][2])) + \
				 wp[0] * (df(mat[1][0], mat[1][1]) + df(mat[2][0], mat[2][1]) + df(mat[1][2], mat[1][3]) + df(mat[2][2], mat[2][3])) + \
				 wp[2] * (df(mat[1][0], mat[1][2]) + df(mat[1][1], mat[1][3]) + df(mat[2][0], mat[2][2]) + df(mat[2][1], mat[2][3]));

	return (hvw1 - hvw2);
}


static inline __attribute((always_inline))
void bilinear_filter(float u, float v,
	const float r[][4], const float g[][4], const float b[][4], const float a[][4],
	float & r1, float &  g1, float &  b1, float &  a1,
	float &  r2, float & g2, float & b2, float & a2)
{
	r1 = u*(r[0][3] + r[3][0]) + v*(r[1][2] + r[2][1]);
	g1 = u*(g[0][3] + g[3][0]) + v*(g[1][2] + g[2][1]);
	b1 = u*(b[0][3] + b[3][0]) + v*(b[1][2] + b[2][1]);
	a1 = u*(a[0][3] + a[3][0]) + v*(a[1][2] + a[2][1]);
	r2 = u*(r[0][0] + r[3][3]) + v*(r[1][1] + r[2][2]);
	g2 = u*(g[0][0] + g[3][3]) + v*(g[1][1] + g[2][2]);
	b2 = u*(b[0][0] + b[3][3]) + v*(b[1][1] + b[2][2]);
	a2 = u*(a[0][0] + a[3][3]) + v*(a[1][1] + a[2][2]);
}

///////////////////////// Super-xBR scaling
// perform super-xbr (fast shader version) scaling by factor f=2 only.
template<int f>
void scaleSuperXBRT(uint32_t* data, uint32_t* out, int w, int h) {
	int outw = w*f, outh = h*f;

	float wp[6] = { 2.0f, 1.0f, -1.0f, 4.0f, -1.0f, 1.0f };

	// First Pass
	for (int y = 0; y < outh; y += f) {
		for (int x = 0; x < outw; x += f) {
			float r[4][4], g[4][4], b[4][4], a[4][4], Y[4][4];
			int cx = x / f, cy = y / f; // central pixels on original images
			// sample supporting pixels in original image
			for (int sx = -1; sx <= 2; ++sx) {
				for (int sy = -1; sy <= 2; ++sy) {
					// clamp pixel locations
					int csy = clamp(sy + cy, 0, h - 1);
					int csx = clamp(sx + cx, 0, w - 1);
					// sample & add weighted components
					uint32_t sample = data[csy*w + csx];
					r[sx + 1][sy + 1] = R(sample);
					g[sx + 1][sy + 1] = G(sample);
					b[sx + 1][sy + 1] = B(sample);
					a[sx + 1][sy + 1] = A(sample);
					Y[sx + 1][sy + 1] = getLuminescence(r[sx + 1][sy + 1], g[sx + 1][sy + 1], b[sx + 1][sy + 1]);
				}
			}
			float d_edge = diagonal_edge(Y, &wp[0]);

			float r1, g1, b1, a1, r2, g2, b2, a2, rf, gf, bf, af;
			bilinear_filter(w1, w2, r, g, b, a, r1, g1, b1, a1, r2, g2, b2, a2);

			// generate and write result
			if (d_edge <= 0.0f) { rf = r1; gf = g1; bf = b1; af = a1; }
			else { rf = r2; gf = g2; bf = b2; af = a2; }
			// anti-ringing, clamp.
			out[y*outw + x] = out[y*outw + x + 1] = out[(y + 1)*outw + x] = data[cy*w + cx];
			out[(y+1)*outw + x+1] = toColor(r, rf, g, gf, b, bf, a, af);
		}
	}

	// Second Pass
	wp[0] = 2.0f;
	wp[1] = 0.0f;
	wp[2] = 0.0f;
	wp[3] = 0.0f;
	wp[4] = 0.0f;
	wp[5] = 0.0f;

	for (int y = 0; y < outh; y += f) {
		for (int x = 0; x < outw; x += f) {
			float r[4][4], g[4][4], b[4][4], a[4][4], Y[4][4];
			// sample supporting pixels in original image
			for (int sx = -1; sx <= 2; ++sx) {
				for (int sy = -1; sy <= 2; ++sy) {
					// clamp pixel locations
					int csy = clamp(sx - sy + y, 0, f*h - 1);
					int csx = clamp(sx + sy + x, 0, f*w - 1);
					// sample & add weighted components
					uint32_t sample = out[csy*outw + csx];
					r[sx + 1][sy + 1] = R(sample);
					g[sx + 1][sy + 1] = G(sample);
					b[sx + 1][sy + 1] = B(sample);
					a[sx + 1][sy + 1] = A(sample);
					Y[sx + 1][sy + 1] = getLuminescence(r[sx + 1][sy + 1], g[sx + 1][sy + 1], b[sx + 1][sy + 1]);
				}
			}

			float d_edge = diagonal_edge(Y, &wp[0]);
			float r1, g1, b1, a1, r2, g2, b2, a2, rf, gf, bf, af;
			bilinear_filter(w3, w4, r, g, b, a,  r1, g1, b1, a1, r2, g2, b2, a2);

			// generate and write result
			if (d_edge <= 0.0f) { rf = r1; gf = g1; bf = b1; af = a1; }
			else { rf = r2; gf = g2; bf = b2; af = a2; }

			float min_r_sample = min4(r[1][1], r[2][1], r[1][2], r[2][2]);
			float min_g_sample = min4(g[1][1], g[2][1], g[1][2], g[2][2]);
			float min_b_sample = min4(b[1][1], b[2][1], b[1][2], b[2][2]);
			float min_a_sample = min4(a[1][1], a[2][1], a[1][2], a[2][2]);
			float max_r_sample = max4(r[1][1], r[2][1], r[1][2], r[2][2]);
			float max_g_sample = max4(g[1][1], g[2][1], g[1][2], g[2][2]);
			float max_b_sample = max4(b[1][1], b[2][1], b[1][2], b[2][2]);
			float max_a_sample = max4(a[1][1], a[2][1], a[1][2], a[2][2]);
			out[y*outw + x+1] = toColor(min_r_sample, max_r_sample, rf, min_g_sample, max_g_sample, gf, min_b_sample, max_b_sample, bf, min_a_sample, max_a_sample, af);

			for (int sx = -1; sx <= 2; ++sx) {
				for (int sy = -1; sy <= 2; ++sy) {
					// clamp pixel locations
					int csy = clamp(sx - sy + 1 + y, 0, f*h - 1);
					int csx = clamp(sx + sy - 1 + x, 0, f*w - 1);
					// sample & add weighted components
					uint32_t sample = out[csy*outw + csx];
					r[sx + 1][sy + 1] = R(sample);
					g[sx + 1][sy + 1] = G(sample);
					b[sx + 1][sy + 1] = B(sample);
					a[sx + 1][sy + 1] = A(sample);
					Y[sx + 1][sy + 1] = getLuminescence(r[sx + 1][sy + 1], g[sx + 1][sy + 1], b[sx + 1][sy + 1]);
				}
			}
			d_edge = diagonal_edge(Y, &wp[0]);
			bilinear_filter(w3, w4, r, g, b, a,  r1, g1, b1, a1, r2, g2, b2, a2);
			// generate and write result
			if (d_edge <= 0.0f) { rf = r1; gf = g1; bf = b1; af = a1; }
			else { rf = r2; gf = g2; bf = b2; af = a2; }

			out[(y+1)*outw + x] = toColor(min_r_sample, max_r_sample, rf, min_g_sample, max_g_sample, gf, min_b_sample, max_b_sample, bf, min_a_sample, max_a_sample, af);
		}
	}

	// Third Pass
	wp[0] =  2.0f;
	wp[1] =  1.0f;
	wp[2] = -1.0f;
	wp[3] =  4.0f;
	wp[4] = -1.0f;
	wp[5] =  1.0f;

	for (int y = outh - 1; y >= 0; --y) {
		for (int x = outw - 1; x >= 0; --x) {
			float r[4][4], g[4][4], b[4][4], a[4][4], Y[4][4];
			for (int sx = -2; sx <= 1; ++sx) {
				for (int sy = -2; sy <= 1; ++sy) {
					// clamp pixel locations
					int csy = clamp(sy + y, 0, f*h - 1);
					int csx = clamp(sx + x, 0, f*w - 1);
					// sample & add weighted components
					uint32_t sample = out[csy*outw + csx];
					r[sx + 2][sy + 2] = R(sample);
					g[sx + 2][sy + 2] = G(sample);
					b[sx + 2][sy + 2] = B(sample);
					a[sx + 2][sy + 2] = A(sample);
					Y[sx + 2][sy + 2] = getLuminescence(r[sx + 2][sy + 2], g[sx + 2][sy + 2], b[sx + 2][sy + 2]);
				}
			}
			float d_edge = diagonal_edge(Y, &wp[0]);
			float r1, g1, b1, a1, r2, g2, b2, a2, rf, gf, bf, af;
			bilinear_filter(w3, w4, r, g, b, a,  r1, g1, b1, a1, r2, g2, b2, a2);

			// generate and write result
			if (d_edge <= 0.0f) { rf = r1; gf = g1; bf = b1; af = a1; }
			else { rf = r2; gf = g2; bf = b2; af = a2; }

			out[y*outw + x] = toColor(r, rf, g, gf, b, bf, a, af);
		}
	}

}

//// *** Super-xBR code ends here - MIT LICENSE *** ///

void __attribute((flatten)) scaleSuperXbr(uint32_t *data, uint32_t *out, int w, int h)
{
	scaleSuperXBRT<2>(data, out, w, h);
}

