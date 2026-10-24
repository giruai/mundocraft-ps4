// SPDX-License-Identifier: MIT
// The pinned SDK libc's wide-string objects use 16-bit units, while this
// target and its prebuilt libc++ use 32-bit wchar_t. Keep these routines in
// one translation unit compiled with -fno-builtin, so libc++ wide strings
// never call the incompatible libc implementations.
#include <cstddef>
#include <cstdint>

static_assert(sizeof(wchar_t) == 4, "Luanti and SDK libc++ require 32-bit wchar_t");

extern "C" {

std::size_t wcslen(const wchar_t *s)
{
	std::size_t n = 0;
	while (s[n])
		++n;
	return n;
}

std::size_t wcsnlen(const wchar_t *s, std::size_t maxlen)
{
	std::size_t n = 0;
	while (n < maxlen && s[n])
		++n;
	return n;
}

wchar_t *wmemcpy(wchar_t *dst, const wchar_t *src, std::size_t n)
{
	for (std::size_t i = 0; i < n; ++i)
		dst[i] = src[i];
	return dst;
}

wchar_t *wmemmove(wchar_t *dst, const wchar_t *src, std::size_t n)
{
	if (dst == src || n == 0)
		return dst;
	const std::uintptr_t d = reinterpret_cast<std::uintptr_t>(dst);
	const std::uintptr_t s = reinterpret_cast<std::uintptr_t>(src);
	if (d < s || (d - s) / sizeof(wchar_t) >= n) {
		for (std::size_t i = 0; i < n; ++i)
			dst[i] = src[i];
	} else {
		for (std::size_t i = n; i != 0; --i)
			dst[i - 1] = src[i - 1];
	}
	return dst;
}

wchar_t *wmemset(wchar_t *dst, wchar_t value, std::size_t n)
{
	for (std::size_t i = 0; i < n; ++i)
		dst[i] = value;
	return dst;
}

int wmemcmp(const wchar_t *a, const wchar_t *b, std::size_t n)
{
	for (std::size_t i = 0; i < n; ++i) {
		if (a[i] < b[i])
			return -1;
		if (a[i] > b[i])
			return 1;
	}
	return 0;
}

wchar_t *wmemchr(const wchar_t *s, wchar_t value, std::size_t n)
{
	for (std::size_t i = 0; i < n; ++i)
		if (s[i] == value)
			return const_cast<wchar_t *>(s + i);
	return nullptr;
}

int wcscmp(const wchar_t *a, const wchar_t *b)
{
	while (*a && *a == *b) {
		++a;
		++b;
	}
	return *a < *b ? -1 : *a > *b ? 1 : 0;
}

int wcsncmp(const wchar_t *a, const wchar_t *b, std::size_t n)
{
	for (std::size_t i = 0; i < n; ++i) {
		if (a[i] != b[i])
			return a[i] < b[i] ? -1 : 1;
		if (a[i] == 0)
			return 0;
	}
	return 0;
}

wchar_t *wcschr(const wchar_t *s, wchar_t value)
{
	for (;;) {
		if (*s == value)
			return const_cast<wchar_t *>(s);
		if (!*s)
			return nullptr;
		++s;
	}
}

wchar_t *wcsrchr(const wchar_t *s, wchar_t value)
{
	const wchar_t *last = nullptr;
	for (;;) {
		if (*s == value)
			last = s;
		if (!*s)
			return const_cast<wchar_t *>(last);
		++s;
	}
}

wchar_t *wcscpy(wchar_t *dst, const wchar_t *src)
{
	wchar_t *result = dst;
	do {
		*dst++ = *src;
	} while (*src++);
	return result;
}

wchar_t *wcsncpy(wchar_t *dst, const wchar_t *src, std::size_t n)
{
	std::size_t i = 0;
	for (; i < n && src[i]; ++i)
		dst[i] = src[i];
	for (; i < n; ++i)
		dst[i] = 0;
	return dst;
}

wchar_t *wcscat(wchar_t *dst, const wchar_t *src)
{
	wchar_t *end = dst;
	while (*end)
		++end;
	do {
		*end++ = *src;
	} while (*src++);
	return dst;
}

wchar_t *wcsncat(wchar_t *dst, const wchar_t *src, std::size_t n)
{
	wchar_t *end = dst;
	while (*end)
		++end;
	std::size_t i = 0;
	while (i < n && src[i]) {
		end[i] = src[i];
		++i;
	}
	end[i] = 0;
	return dst;
}

} // extern "C"
