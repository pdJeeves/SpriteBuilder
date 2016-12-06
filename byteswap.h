#ifndef BYTESWAP_H
#define BYTESWAP_H

static inline
long long __attribute((const)) __attribute((always_inline)) _byte_swap(long long x)
{
	x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
	x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
	x = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
	return x;
}

static inline
int __attribute((const)) __attribute((always_inline)) _byte_swap(int x)
{
	x = (x & 0x0000FFFF) << 16 | (x & 0xFFFF0000) >> 16;
	x = (x & 0x00FF00FF) << 8  | (x & 0xFF00FF00) >> 8;
	return x;
}

static inline
short __attribute((const)) __attribute((always_inline)) _byte_swap(short t)
{
	return (t & 0xFF00) >> 8 | (t & 0x00FF) << 8;
}

static inline
unsigned long long __attribute((const)) __attribute((always_inline)) _byte_swap(unsigned long long x)
{
	return (unsigned long long) _byte_swap((long long) x);
}

static inline
unsigned int __attribute((const)) __attribute((always_inline)) _byte_swap(unsigned int x)
{
	return (unsigned int) _byte_swap((int) x);
}

static inline
unsigned short __attribute((const)) __attribute((always_inline)) _byte_swap(unsigned short x)
{
	return (unsigned short) _byte_swap((short) x);
}

static inline
bool __attribute((const)) __attribute((always_inline)) isLittleEndian()
{
	int i = 1;
	return (int)*((unsigned char *)&i)==1;
}

template<typename T>
inline
T __attribute((const)) __attribute((always_inline)) byte_swap(T t)
{
	return isLittleEndian() ? t : _byte_swap(t);
}

typedef unsigned char BYTE;
typedef unsigned short WORD;

static
unsigned int __attribute((unused)) from565(WORD color)
{
	int r = ((color >> 11) << 3) & 0xFF;
	int g = ((color >> 5) << 2) & 0xFF;
	int b = color << 3 & 0xFF;
	return (((((0xFF << 8) | r) << 8) | g) << 8) | b;
}

#endif // BYTESWAP_H
