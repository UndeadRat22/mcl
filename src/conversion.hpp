#pragma once
#include <vector>
#include <cybozu/itoa.hpp>
#include <cybozu/atoi.hpp>
#include <mcl/util.hpp>
/**
	@file
	@brief convertion from T[] to str2, str16
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/

namespace mcl { namespace fp {

/*
	convert little endian x[0, xn) to buf
	return written size if success else 0
	data is buf[bufSize - retval, bufSize)
	start "0x" if withPrefix
*/
template<class T>
size_t arrayToHex(char *buf, size_t bufSize, const T *x, size_t n, bool withPrefix = false)
{
	size_t fullN = 0;
	if (n > 1) {
		size_t pos = n - 1;
		while (pos > 0) {
			if (x[pos]) break;
			pos--;
		}
		if (pos > 0) fullN = pos;
	}
	const T v = n == 0 ? 0 : x[fullN];
	const size_t topLen = cybozu::getHexLength(v);
	const size_t startPos = withPrefix ? 2 : 0;
	const size_t lenT = sizeof(T) * 2;
	const size_t totalSize = startPos + fullN * lenT + topLen;
	if (totalSize > bufSize) return 0;
	char *const top = buf + bufSize - totalSize;
	if (withPrefix) {
		top[0] = '0';
		top[1] = 'x';
	}
	cybozu::itohex(&top[startPos], topLen, v, false);
	for (size_t i = 0; i < fullN; i++) {
		cybozu::itohex(&top[startPos + topLen + i * lenT], lenT, x[fullN - 1 - i], false);
	}
	return totalSize;
}

/*
	convert little endian x[0, xn) to buf
	return written size if success else 0
	data is buf[bufSize - retval, bufSize)
	start "0b" if withPrefix
*/
template<class T>
size_t arrayToBin(char *buf, size_t bufSize, const T *x, size_t n, bool withPrefix)
{
	size_t fullN = 0;
	if (n > 1) {
		size_t pos = n - 1;
		while (pos > 0) {
			if (x[pos]) break;
			pos--;
		}
		if (pos > 0) fullN = pos;
	}
	const T v = n == 0 ? 0 : x[fullN];
	const size_t topLen = cybozu::getBinLength(v);
	const size_t startPos = withPrefix ? 2 : 0;
	const size_t lenT = sizeof(T) * 8;
	const size_t totalSize = startPos + fullN * lenT + topLen;
	if (totalSize > bufSize) return 0;
	char *const top = buf + bufSize - totalSize;
	if (withPrefix) {
		top[0] = '0';
		top[1] = 'b';
	}
	cybozu::itobin(&top[startPos], topLen, v);
	for (size_t i = 0; i < fullN; i++) {
		cybozu::itobin(&top[startPos + topLen + i * lenT], lenT, x[fullN - 1 - i]);
	}
	return totalSize;
}

/*
	convert hex string to x[0..xn)
	hex string = [0-9a-fA-F]+
*/
template<class T>
void fromStr16(T *x, size_t xn, const char *str, size_t strLen)
{
	if (strLen == 0) throw cybozu::Exception("fp:fromStr16:strLen is zero");
	const size_t unitLen = sizeof(T) * 2;
	const size_t q = strLen / unitLen;
	const size_t r = strLen % unitLen;
	const size_t requireSize = q + (r ? 1 : 0);
	if (xn < requireSize) throw cybozu::Exception("fp:fromStr16:short size") << xn << requireSize;
	for (size_t i = 0; i < q; i++) {
		bool b;
		x[i] = cybozu::hextoi(&b, &str[r + (q - 1 - i) * unitLen], unitLen);
		if (!b) throw cybozu::Exception("fp:fromStr16:bad char") << cybozu::exception::makeString(str, strLen);
	}
	if (r) {
		bool b;
		x[q] = cybozu::hextoi(&b, str, r);
		if (!b) throw cybozu::Exception("fp:fromStr16:bad char") << cybozu::exception::makeString(str, strLen);
	}
	for (size_t i = requireSize; i < xn; i++) x[i] = 0;
}

/*
	convert hex string to x[0..xn)
	hex string = [0-9a-fA-F]+
*/
template<class UT>
inline size_t hexToArray(UT *x, size_t maxN, const char *buf, size_t bufSize)
{
	if (bufSize == 0) return 0;
	const size_t unitLen = sizeof(UT) * 2;
	const size_t q = bufSize / unitLen;
	const size_t r = bufSize % unitLen;
	const size_t requireSize = q + (r ? 1 : 0);
	if (maxN < requireSize) return 0;
	for (size_t i = 0; i < q; i++) {
		bool b;
		x[i] = cybozu::hextoi(&b, &buf[r + (q - 1 - i) * unitLen], unitLen);
		if (!b) return 0;
	}
	if (r) {
		bool b;
		x[q] = cybozu::hextoi(&b, buf, r);
		if (!b) return 0;
	}
	return requireSize;
}

namespace local {

/*
	q = x[] / x
	@retval r = x[] % x
	@note accept q == x
*/
inline uint32_t divU32(uint32_t *q, const uint32_t *x, size_t xn, uint32_t y)
{
	if (xn == 0) return 0;
	uint32_t r = 0;
	for (int i = (int)xn - 1; i >= 0; i--) {
		uint64_t t = (uint64_t(r) << 32) | x[i];
		q[i] = uint32_t(t / y);
		r = uint32_t(t % y);
	}
	return r;
}

/*
	z[0, xn) = x[0, xn) * y
	return z[xn]
	@note accept z == x
*/
inline uint32_t mulU32(uint32_t *z, const uint32_t *x, size_t xn, uint32_t y)
{
	uint32_t H = 0;
	for (size_t i = 0; i < xn; i++) {
		uint32_t t = H;
		uint64_t v = uint64_t(x[i]) * y;
		uint32_t L = uint32_t(v);
		H = uint32_t(v >> 32);
		z[i] = t + L;
		if (z[i] < t) {
			H++;
		}
	}
	return H;
}

/*
	x[0, xn) += y
	return 1 if overflow else 0
*/
inline uint32_t addU32(uint32_t *x, size_t xn, uint32_t y)
{
	uint32_t t = x[0] + y;
	x[0] = t;
	if (t >= y) return 0;
	for (size_t i = 1; i < xn; i++) {
		t = x[i] + 1;
		x[i] = t;
		if (t != 0) return 0;
	}
	return 1;
}

inline uint32_t decToU32(const char *p, size_t size, bool *pb)
{
	assert(0 < size && size <= 9);
	uint32_t x = 0;
	for (size_t i = 0; i < size; i++) {
		char c = p[i];
		if (c < '0' || c > '9') {
			*pb = false;
			return 0;
		}
		x = x * 10 + uint32_t(c - '0');
	}
	*pb = true;
	return x;
}

} // mcl::fp::local

/*
	little endian x[0, xn) to buf
	return written size if success else 0
	data is buf[bufSize - retval, bufSize)
*/
template<class UT>
inline size_t arrayToDec(char *buf, size_t bufSize, const UT *x, size_t xn)
{
	const size_t maxN = 64;
	uint32_t t[maxN];
	if (sizeof(UT) == 8) {
		xn *= 2;
	}
	if (xn > maxN) return 0;
	memcpy(t, x, xn * sizeof(t[0]));

	const size_t width = 9;
	const uint32_t i1e9 = 1000000000U;
	size_t pos = 0;
	for (;;) {
		uint32_t r = local::divU32(t, t, xn, i1e9);
		while (xn > 0 && t[xn - 1] == 0) xn--;
		size_t len = cybozu::itoa_local::uintToDec(buf, bufSize - pos, r);
		if (len == 0) return 0;
		assert(0 < len && len <= width);
		if (xn == 0) return pos + len;
		// fill (width - len) '0'
		for (size_t j = 0; j < width - len; j++) {
			buf[bufSize - pos - width + j] = '0';
		}
		pos += width;
	}
}

/*
	convert buf[0, bufSize) to x[0, num)
	return written num if success else 0
*/
template<class UT>
inline size_t decToArray(UT *_x, size_t maxN, const char *buf, size_t bufSize)
{
	assert(sizeof(UT) == 4 || sizeof(UT) == 8);
	const size_t width = 9;
	const uint32_t i1e9 = 1000000000U;
	if (maxN == 0) return 0;
	if (sizeof(UT) == 8) {
		maxN *= 2;
	}
	uint32_t *x = reinterpret_cast<uint32_t*>(_x);
	size_t xn = 1;
	x[0] = 0;
	while (bufSize > 0) {
		size_t n = bufSize % width;
		if (n == 0) n = width;
		bool b;
		uint32_t v = local::decToU32(buf, n, &b);
		if (!b) return 0;
		uint32_t H = local::mulU32(x, x, xn, i1e9);
		if (H > 0) {
			if (xn == maxN) return 0;
			x[xn++] = H;
		}
		H = local::addU32(x, xn, v);
		if (H > 0) {
			if (xn == maxN) return 0;
			x[xn++] = H;
		}
		buf += n;
		bufSize -= n;
	}
	if (sizeof(UT) == 8 && (xn & 1)) {
		x[xn++] = 0;
	}
	return xn / (sizeof(UT) / 4);
}

} } // mcp::fp
