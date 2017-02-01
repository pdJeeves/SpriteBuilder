#ifndef BYTESWAP_H
#define BYTESWAP_H

#ifdef __GNUC__
#define CONST __attribute((const))
#define ALWAYS_INLINE __attribute((always_inline))
#define UNUSED __attribute((unused))
#define FLATTEN __attribute((flatten))
#define PACK(a, b) a __attribute((packed)) b
#define UNPACK
#else
#define CONST
#define ALWAYS_INLINE
#define UNUSED
#define FLATTEN
#define PACK(a, b) __pragma(pack(push, 1)); a b
#define UNPACK __pragma(pack(pop));
#endif

static inline
long long CONST ALWAYS_INLINE _byte_swap(long long x)
{
	x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
	x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
	x = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
	return x;
}

static inline
int CONST ALWAYS_INLINE _byte_swap(int x)
{
	x = (x & 0x0000FFFF) << 16 | (x & 0xFFFF0000) >> 16;
	x = (x & 0x00FF00FF) << 8  | (x & 0xFF00FF00) >> 8;
	return x;
}

static inline
short CONST ALWAYS_INLINE _byte_swap(short t)
{
	return (t & 0xFF00) >> 8 | (t & 0x00FF) << 8;
}

static inline
unsigned long long CONST ALWAYS_INLINE _byte_swap(unsigned long long x)
{
	return (unsigned long long) _byte_swap((long long) x);
}

static inline
unsigned int CONST ALWAYS_INLINE _byte_swap(unsigned int x)
{
	return (unsigned int) _byte_swap((int) x);
}

static inline
unsigned short CONST ALWAYS_INLINE _byte_swap(unsigned short x)
{
	return (unsigned short) _byte_swap((short) x);
}

static inline
bool CONST ALWAYS_INLINE isLittleEndian()
{
	int i = 1;
	return (int)*((unsigned char *)&i)==1;
}

template<typename T>
inline
T CONST ALWAYS_INLINE byte_swap(T t)
{
	return isLittleEndian() ? t : _byte_swap(t);
}

typedef unsigned char BYTE;
typedef unsigned short WORD;

static
unsigned int UNUSED from565(WORD color)
{
	int r = ((color >> 11) << 3) & 0xFF;
	int g = ((color >> 5) << 2) & 0xFF;
	int b = color << 3 & 0xFF;
	return (((((0xFF << 8) | r) << 8) | g) << 8) | b;
}

#endif // BYTESWAP_H
