#include "shared/cube.h"

///////////////////////// cryptography /////////////////////////////////

/* Based off the reference implementation of Tiger, a cryptographically
 * secure 192 bit hash function by Ross Anderson and Eli Biham. More info at:
 * http://www.cs.technion.ac.il/~biham/Reports/Tiger/
 */

enum { TIGER_PASSES = 3 };

namespace tiger {
using chunk = unsigned long long;

union hashval {
	uchar bytes[3 * 8];
	chunk chunks[3];
};

chunk sboxes[4 * 256];

void compress(const chunk* str, chunk state[3]) {
	chunk a, b, c;
	chunk aa, bb, cc;
	chunk x0, x1, x2, x3, x4, x5, x6, x7;

	a = state[0];
	b = state[1];
	c = state[2];

	x0 = str[0];
	x1 = str[1];
	x2 = str[2];
	x3 = str[3];
	x4 = str[4];
	x5 = str[5];
	x6 = str[6];
	x7 = str[7];

	aa = a;
	bb = b;
	cc = c;

	loop(pass_no, TIGER_PASSES) {
		if (pass_no) {
			x0 -= x7 ^ 0xA5A5A5A5A5A5A5A5ULL;
			x1 ^= x0;
			x2 += x1;
			x3 -= x2 ^ ((~x1) << 19);
			x4 ^= x3;
			x5 += x4;
			x6 -= x5 ^ ((~x4) >> 23);
			x7 ^= x6;
			x0 += x7;
			x1 -= x0 ^ ((~x7) << 19);
			x2 ^= x1;
			x3 += x2;
			x4 -= x3 ^ ((~x2) >> 23);
			x5 ^= x4;
			x6 += x5;
			x7 -= x6 ^ 0x0123456789ABCDEFULL;
		}

#define sb1 (sboxes)
#define sb2 (sboxes + 256)
#define sb3 (sboxes + 256 * 2)
#define sb4 (sboxes + 256 * 3)

#define round(a, b, c, x)                                                                             \
	c ^= x;                                                                                           \
	a -= sb1[((c) >> (0 * 8)) & 0xFF] ^ sb2[((c) >> (2 * 8)) & 0xFF] ^ sb3[((c) >> (4 * 8)) & 0xFF] ^ \
		sb4[((c) >> (6 * 8)) & 0xFF];                                                                 \
	b += sb4[((c) >> (1 * 8)) & 0xFF] ^ sb3[((c) >> (3 * 8)) & 0xFF] ^ sb2[((c) >> (5 * 8)) & 0xFF] ^ \
		sb1[((c) >> (7 * 8)) & 0xFF];                                                                 \
	b *= mul;

		uint mul = !pass_no ? 5 : (pass_no == 1 ? 7 : 9);
		round(a, b, c, x0) round(b, c, a, x1) round(c, a, b, x2) round(a, b, c, x3) round(b, c, a, x4)
			round(c, a, b, x5) round(a, b, c, x6) round(b, c, a, x7)

				chunk tmp = a;
		a = c;
		c = b;
		b = tmp;
	}

	a ^= aa;
	b -= bb;
	c += cc;

	state[0] = a;
	state[1] = b;
	state[2] = c;
}

void gensboxes() {
	const char* str = "Tiger - A Fast New Hash Function, by Ross Anderson and Eli Biham";
	chunk state[3] = {0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL, 0xF096A5B4C3B2E187ULL};
	uchar temp[64];

	if (!islittleendian())
		loopj(64) temp[j ^ 7] = str[j];
	else
		loopj(64) temp[j] = str[j];
	loopi(1024) loop(col, 8)((uchar*)&sboxes[i])[col] = i & 0xFF;

	int abc = 2;
	loop(pass, 5) loopi(256) for (int sb = 0; sb < 1024; sb += 256) {
		abc++;
		if (abc >= 3) {
			abc = 0;
			compress((chunk*)temp, state);
		}
		loop(col, 8) {
			uchar val = ((uchar*)&sboxes[sb + i])[col];
			((uchar*)&sboxes[sb + i])[col] = ((uchar*)&sboxes[sb + ((uchar*)&state[abc])[col]])[col];
			((uchar*)&sboxes[sb + ((uchar*)&state[abc])[col]])[col] = val;
		}
	}
}

void hash(const uchar* str, int length, hashval& val) {
	static bool init = false;
	if (!init) {
		gensboxes();
		init = true;
	}

	uchar temp[64];

	val.chunks[0] = 0x0123456789ABCDEFULL;
	val.chunks[1] = 0xFEDCBA9876543210ULL;
	val.chunks[2] = 0xF096A5B4C3B2E187ULL;

	int i = length;
	for (; i >= 64; i -= 64, str += 64) {
		if (!islittleendian()) {
			loopj(64) temp[j ^ 7] = str[j];
			compress((chunk*)temp, val.chunks);
		} else
			compress((chunk*)str, val.chunks);
	}

	int j;
	if (!islittleendian()) {
		for (j = 0; j < i; j++)
			temp[j ^ 7] = str[j];
		temp[j ^ 7] = 0x01;
		while (++j & 7)
			temp[j ^ 7] = 0;
	} else {
		for (j = 0; j < i; j++)
			temp[j] = str[j];
		temp[j] = 0x01;
		while (++j & 7)
			temp[j] = 0;
	}

	if (j > 56) {
		while (j < 64)
			temp[j++] = 0;
		compress((chunk*)temp, val.chunks);
		j = 0;
	}
	while (j < 56)
		temp[j++] = 0;
	*(chunk*)(temp + 56) = (chunk)length << 3;
	compress((chunk*)temp, val.chunks);
	if (!islittleendian()) {
		loopk(3) {
			uchar* c = &val.bytes[k * sizeof(chunk)];
			loopl(sizeof(chunk) / 2) swap(c[l], c[sizeof(chunk) - 1 - l]);
		}
	}
}
}  // namespace tiger

/* Elliptic curve cryptography based on NIST DSS prime curves. */

enum { BI_DIGIT_BITS = 16 };
#define BI_DIGIT_MASK ((1 << BI_DIGIT_BITS) - 1)

template <int BI_DIGITS>
struct bigint {
	using digit = ushort;
	using dbldigit = uint;

	int len;
	digit digits[BI_DIGITS];

	bigint() = default;
	bigint(digit n) {
		if (n) {
			len = 1;
			digits[0] = n;
		} else
			len = 0;
	}
	bigint(const char* s) {
		parse(s);
	}
	template <int Y_DIGITS>
	bigint(const bigint<Y_DIGITS>& y) {
		*this = y;
	}

	static auto parsedigits(ushort* digits, int maxlen, const char* s) -> int {
		int slen = 0;
		while (isxdigit(s[slen]))
			slen++;
		int len = (slen + 2 * sizeof(ushort) - 1) / (2 * sizeof(ushort));
		if (len > maxlen)
			return 0;
		memset(digits, 0, len * sizeof(ushort));
		loopi(slen) {
			int c = s[slen - i - 1];
			if (isalpha(c))
				c = toupper(c) - 'A' + 10;
			else if (isdigit(c))
				c -= '0';
			else
				return 0;
			digits[i / (2 * sizeof(ushort))] |= c << (4 * (i % (2 * sizeof(ushort))));
		}
		return len;
	}

	void parse(const char* s) {
		len = parsedigits(digits, BI_DIGITS, s);
		shrink();
	}

	void zero() {
		len = 0;
	}

	void print(stream* out) const {
		vector<char> buf;
		printdigits(buf);
		out->write(buf.getbuf(), buf.length());
	}

	void printdigits(vector<char>& buf) const {
		loopi(len) {
			digit d = digits[len - i - 1];
			loopj(BI_DIGIT_BITS / 4) {
				uint shift = BI_DIGIT_BITS - (j + 1) * 4;
				int val = (d >> shift) & 0xF;
				if (val < 10)
					buf.add('0' + val);
				else
					buf.add('a' + val - 10);
			}
		}
	}

	template <int Y_DIGITS>
	auto operator=(const bigint<Y_DIGITS>& y) -> bigint& {
		len = y.len;
		memcpy(digits, y.digits, len * sizeof(digit));
		return *this;
	}

	[[nodiscard]] auto iszero() const -> bool {
		return !len;
	}
	[[nodiscard]] auto isone() const -> bool {
		return len == 1 && digits[0] == 1;
	}

	[[nodiscard]] auto numbits() const -> int {
		if (!len)
			return 0;
		int bits = len * BI_DIGIT_BITS;
		digit last = digits[len - 1], mask = 1 << (BI_DIGIT_BITS - 1);
		while (mask) {
			if (last & mask)
				return bits;
			bits--;
			mask >>= 1;
		}
		return 0;
	}

	[[nodiscard]] auto hasbit(int n) const -> bool {
		return n / BI_DIGIT_BITS < len && ((digits[n / BI_DIGIT_BITS] >> (n % BI_DIGIT_BITS)) & 1);
	}

	[[nodiscard]] auto morebits(int n) const -> bool {
		return len > n / BI_DIGIT_BITS;
	}

	template <int X_DIGITS, int Y_DIGITS>
	auto add(const bigint<X_DIGITS>& x, const bigint<Y_DIGITS>& y) -> bigint& {
		dbldigit carry = 0;
		int maxlen = max(x.len, y.len), i;
		for (i = 0; i < y.len || carry; i++) {
			carry += (i < x.len ? (dbldigit)x.digits[i] : 0) + (i < y.len ? (dbldigit)y.digits[i] : 0);
			digits[i] = (digit)carry;
			carry >>= BI_DIGIT_BITS;
		}
		if (i < x.len && this != &x)
			memcpy(&digits[i], &x.digits[i], (x.len - i) * sizeof(digit));
		len = max(i, maxlen);
		return *this;
	}
	template <int Y_DIGITS>
	auto add(const bigint<Y_DIGITS>& y) -> bigint& {
		return add(*this, y);
	}

	template <int X_DIGITS, int Y_DIGITS>
	auto sub(const bigint<X_DIGITS>& x, const bigint<Y_DIGITS>& y) -> bigint& {
		dbldigit borrow = 0;
		int i;
		for (i = 0; i < y.len || borrow; i++) {
			borrow = (1 << BI_DIGIT_BITS) + (dbldigit)x.digits[i] - (i < y.len ? (dbldigit)y.digits[i] : 0) - borrow;
			digits[i] = (digit)borrow;
			borrow = (borrow >> BI_DIGIT_BITS) ^ 1;
		}
		if (i < x.len && this != &x)
			memcpy(&digits[i], &x.digits[i], (x.len - i) * sizeof(digit));
		len = x.len;
		shrink();
		return *this;
	}
	template <int Y_DIGITS>
	auto sub(const bigint<Y_DIGITS>& y) -> bigint& {
		return sub(*this, y);
	}

	void shrink() {
		while (len && !digits[len - 1])
			len--;
	}
	void shrinkdigits(int n) {
		len = n;
		shrink();
	}
	void shrinkbits(int n) {
		shrinkdigits(n / BI_DIGIT_BITS);
	}

	template <int Y_DIGITS>
	void copyshrinkdigits(const bigint<Y_DIGITS>& y, int n) {
		len = min(y.len, n);
		memcpy(digits, y.digits, len * sizeof(digit));
		shrink();
	}
	template <int Y_DIGITS>
	void copyshrinkbits(const bigint<Y_DIGITS>& y, int n) {
		copyshrinkdigits(y, n / BI_DIGIT_BITS);
	}

	template <int X_DIGITS, int Y_DIGITS>
	auto mul(const bigint<X_DIGITS>& x, const bigint<Y_DIGITS>& y) -> bigint& {
		if (!x.len || !y.len) {
			len = 0;
			return *this;
		}
		memset(digits, 0, y.len * sizeof(digit));
		loopi(x.len) {
			dbldigit carry = 0;
			loopj(y.len) {
				carry += (dbldigit)x.digits[i] * (dbldigit)y.digits[j] + (dbldigit)digits[i + j];
				digits[i + j] = (digit)carry;
				carry >>= BI_DIGIT_BITS;
			}
			digits[i + y.len] = carry;
		}
		len = x.len + y.len;
		shrink();
		return *this;
	}

	auto rshift(int n) -> bigint& {
		if (!len || n <= 0)
			return *this;
		if (n >= len * BI_DIGIT_BITS) {
			len = 0;
			return *this;
		}
		int dig = (n - 1) / BI_DIGIT_BITS;
		n = ((n - 1) % BI_DIGIT_BITS) + 1;
		auto carry = digit(digits[dig] >> n);
		for (int i = dig + 1; i < len; i++) {
			digit tmp = digits[i];
			digits[i - dig - 1] = digit((tmp << (BI_DIGIT_BITS - n)) | carry);
			carry = digit(tmp >> n);
		}
		digits[len - dig - 1] = carry;
		len -= dig + (n / BI_DIGIT_BITS);
		shrink();
		return *this;
	}

	auto lshift(int n) -> bigint& {
		if (!len || n <= 0)
			return *this;
		int dig = n / BI_DIGIT_BITS;
		n %= BI_DIGIT_BITS;
		digit carry = 0;
		loopirev(len) {
			digit tmp = digits[i];
			digits[i + dig] = digit((tmp << n) | carry);
			carry = digit(tmp >> (BI_DIGIT_BITS - n));
		}
		len += dig;
		if (carry)
			digits[len++] = carry;
		if (dig)
			memset(digits, 0, dig * sizeof(digit));
		return *this;
	}

	void zerodigits(int i, int n) {
		memset(&digits[i], 0, n * sizeof(digit));
	}
	void zerobits(int i, int n) {
		zerodigits(i / BI_DIGIT_BITS, n / BI_DIGIT_BITS);
	}

	template <int Y_DIGITS>
	void copydigits(int to, const bigint<Y_DIGITS>& y, int from, int n) {
		int avail = min(y.len - from, n);
		memcpy(&digits[to], &y.digits[from], avail * sizeof(digit));
		if (avail < n)
			memset(&digits[to + avail], 0, (n - avail) * sizeof(digit));
	}
	template <int Y_DIGITS>
	void copybits(int to, const bigint<Y_DIGITS>& y, int from, int n) {
		copydigits(to / BI_DIGIT_BITS, y, from / BI_DIGIT_BITS, n / BI_DIGIT_BITS);
	}

	void dupdigits(int to, int from, int n) {
		memcpy(&digits[to], &digits[from], n * sizeof(digit));
	}
	void dupbits(int to, int from, int n) {
		dupdigits(to / BI_DIGIT_BITS, from / BI_DIGIT_BITS, n / BI_DIGIT_BITS);
	}

	template <int Y_DIGITS>
	auto operator==(const bigint<Y_DIGITS>& y) const -> bool {
		if (len != y.len)
			return false;
		loopirev(len) if (digits[i] != y.digits[i]) return false;
		return true;
	}
	template <int Y_DIGITS>
	auto operator!=(const bigint<Y_DIGITS>& y) const -> bool {
		return !(*this == y);
	}
	template <int Y_DIGITS>
	auto operator<(const bigint<Y_DIGITS>& y) const -> bool {
		if (len < y.len)
			return true;
		if (len > y.len)
			return false;
		loopirev(len) {
			if (digits[i] < y.digits[i])
				return true;
			if (digits[i] > y.digits[i])
				return false;
		}
		return false;
	}
	template <int Y_DIGITS>
	auto operator>(const bigint<Y_DIGITS>& y) const -> bool {
		return y < *this;
	}
	template <int Y_DIGITS>
	auto operator<=(const bigint<Y_DIGITS>& y) const -> bool {
		return !(y < *this);
	}
	template <int Y_DIGITS>
	auto operator>=(const bigint<Y_DIGITS>& y) const -> bool {
		return !(*this < y);
	}
};

#define GF_BITS 192
#define GF_DIGITS ((GF_BITS + BI_DIGIT_BITS - 1) / BI_DIGIT_BITS)

using gfint = bigint<((192 + 16 - 1) / 16) + 1>;

/* NIST prime Galois fields.
 * Currently only supports NIST P-192, where P=2^192-2^64-1, and P-256, where P=2^256-2^224+2^192+2^96-1.
 */
struct gfield : gfint {
	static const gfield P;

	gfield() = default;
	gfield(digit n) : gfint(n) {
	}
	gfield(const char* s) : gfint(s) {
	}

	template <int Y_DIGITS>
	gfield(const bigint<Y_DIGITS>& y) : gfint(y) {
	}

	template <int Y_DIGITS>
	auto operator=(const bigint<Y_DIGITS>& y) -> gfield& {
		gfint::operator=(y);
		return *this;
	}

	template <int X_DIGITS, int Y_DIGITS>
	auto add(const bigint<X_DIGITS>& x, const bigint<Y_DIGITS>& y) -> gfield& {
		gfint::add(x, y);
		if (*this >= P)
			gfint::sub(*this, P);
		return *this;
	}
	template <int Y_DIGITS>
	auto add(const bigint<Y_DIGITS>& y) -> gfield& {
		return add(*this, y);
	}

	template <int X_DIGITS>
	auto mul2(const bigint<X_DIGITS>& x) -> gfield& {
		return add(x, x);
	}
	auto mul2() -> gfield& {
		return mul2(*this);
	}

	auto div2() -> gfield& {
		if (hasbit(0))
			gfint::add(*this, P);
		rshift(1);
		return *this;
	}

	template <int X_DIGITS, int Y_DIGITS>
	auto sub(const bigint<X_DIGITS>& x, const bigint<Y_DIGITS>& y) -> gfield& {
		if (x < y) {
			gfint tmp; /* necessary if this==&y, using this instead would clobber y */
			tmp.add(x, P);
			gfint::sub(tmp, y);
		} else
			gfint::sub(x, y);
		return *this;
	}
	template <int Y_DIGITS>
	auto sub(const bigint<Y_DIGITS>& y) -> gfield& {
		return sub(*this, y);
	}

	template <int X_DIGITS>
	auto neg(const bigint<X_DIGITS>& x) -> gfield& {
		gfint::sub(P, x);
		return *this;
	}
	auto neg() -> gfield& {
		return neg(*this);
	}

	template <int X_DIGITS>
	auto square(const bigint<X_DIGITS>& x) -> gfield& {
		return mul(x, x);
	}
	auto square() -> gfield& {
		return square(*this);
	}

	template <int X_DIGITS, int Y_DIGITS>
	auto mul(const bigint<X_DIGITS>& x, const bigint<Y_DIGITS>& y) -> gfield& {
		bigint<X_DIGITS + Y_DIGITS> result;
		result.mul(x, y);
		reduce(result);
		return *this;
	}
	template <int Y_DIGITS>
	auto mul(const bigint<Y_DIGITS>& y) -> gfield& {
		return mul(*this, y);
	}

	template <int RESULT_DIGITS>
	void reduce(const bigint<RESULT_DIGITS>& result) {
#if GF_BITS == 192
		// B = T + S1 + S2 + S3 mod p
		copyshrinkdigits(result, GF_DIGITS);  // T

		if (result.morebits(192)) {
			gfield s;
			s.copybits(0, result, 192, 64);
			s.dupbits(64, 0, 64);
			s.shrinkbits(128);
			add(s);	 // S1

			if (result.morebits(256)) {
				s.zerobits(0, 64);
				s.copybits(64, result, 256, 64);
				s.dupbits(128, 64, 64);
				s.shrinkdigits(GF_DIGITS);
				add(s);	 // S2

				if (result.morebits(320)) {
					s.copybits(0, result, 320, 64);
					s.dupbits(64, 0, 64);
					s.dupbits(128, 0, 64);
					s.shrinkdigits(GF_DIGITS);
					add(s);	 // S3
				}
			}
		} else if (*this >= P)
			gfint::sub(*this, P);
#elif GF_BITS == 256
		// B = T + 2*S1 + 2*S2 + S3 + S4 - D1 - D2 - D3 - D4 mod p
		copyshrinkdigits(result, GF_DIGITS);  // T

		if (result.morebits(256)) {
			gfield s;
			if (result.morebits(352)) {
				s.zerobits(0, 96);
				s.copybits(96, result, 352, 160);
				s.shrinkdigits(GF_DIGITS);
				add(s);
				add(s);	 // S1

				if (result.morebits(384)) {
					// s.zerobits(0, 96);
					s.copybits(96, result, 384, 128);
					s.shrinkbits(224);
					add(s);
					add(s);	 // S2
				}
			}

			s.copybits(0, result, 256, 96);
			s.zerobits(96, 96);
			s.copybits(192, result, 448, 64);
			s.shrinkdigits(GF_DIGITS);
			add(s);	 // S3

			s.copybits(0, result, 288, 96);
			s.copybits(96, result, 416, 96);
			s.dupbits(192, 96, 32);
			s.copybits(224, result, 256, 32);
			s.shrinkdigits(GF_DIGITS);
			add(s);	 // S4

			s.copybits(0, result, 352, 96);
			s.zerobits(96, 96);
			s.copybits(192, result, 256, 32);
			s.copybits(224, result, 320, 32);
			s.shrinkdigits(GF_DIGITS);
			sub(s);	 // D1

			s.copybits(0, result, 384, 128);
			// s.zerobits(128, 64);
			s.copybits(192, result, 288, 32);
			s.copybits(224, result, 352, 32);
			s.shrinkdigits(GF_DIGITS);
			sub(s);	 // D2

			s.copybits(0, result, 416, 96);
			s.copybits(96, result, 256, 96);
			s.zerobits(192, 32);
			s.copybits(224, result, 384, 32);
			s.shrinkdigits(GF_DIGITS);
			sub(s);	 // D3

			s.copybits(0, result, 448, 64);
			s.zerobits(64, 32);
			s.copybits(96, result, 288, 96);
			// s.zerobits(192, 32);
			s.copybits(224, result, 416, 32);
			s.shrinkdigits(GF_DIGITS);
			sub(s);	 // D4
		} else if (*this >= P)
			gfint::sub(*this, P);
#else
#error Unsupported GF
#endif
	}

	template <int X_DIGITS, int Y_DIGITS>
	auto pow(const bigint<X_DIGITS>& x, const bigint<Y_DIGITS>& y) -> gfield& {
		gfield a(x);
		if (y.hasbit(0))
			*this = a;
		else {
			len = 1;
			digits[0] = 1;
			if (!y.len)
				return *this;
		}
		for (int i = 1, j = y.numbits(); i < j; i++) {
			a.square();
			if (y.hasbit(i))
				mul(a);
		}
		return *this;
	}
	template <int Y_DIGITS>
	auto pow(const bigint<Y_DIGITS>& y) -> gfield& {
		return pow(*this, y);
	}

	auto invert(const gfield& x) -> bool {
		if (!x.len)
			return false;
		gfint u(x), v(P), A((gfint::digit)1), C((gfint::digit)0);
		while (!u.iszero()) {
			int ushift = 0, ashift = 0;
			while (!u.hasbit(ushift)) {
				ushift++;
				if (A.hasbit(ashift)) {
					if (ashift) {
						A.rshift(ashift);
						ashift = 0;
					}
					A.add(P);
				}
				ashift++;
			}
			if (ushift)
				u.rshift(ushift);
			if (ashift)
				A.rshift(ashift);
			int vshift = 0, cshift = 0;
			while (!v.hasbit(vshift)) {
				vshift++;
				if (C.hasbit(cshift)) {
					if (cshift) {
						C.rshift(cshift);
						cshift = 0;
					}
					C.add(P);
				}
				cshift++;
			}
			if (vshift)
				v.rshift(vshift);
			if (cshift)
				C.rshift(cshift);
			if (u >= v) {
				u.sub(v);
				if (A < C)
					A.add(P);
				A.sub(C);
			} else {
				v.sub(v, u);
				if (C < A)
					C.add(P);
				C.sub(A);
			}
		}
		if (C >= P)
			gfint::sub(C, P);
		else {
			len = C.len;
			memcpy(digits, C.digits, len * sizeof(digit));
		}
		return true;
	}
	void invert() {
		invert(*this);
	}

	template <int X_DIGITS>
	static auto legendre(const bigint<X_DIGITS>& x) -> int {
		static const gfint Psub1div2(gfint(P).sub(bigint<1>(1)).rshift(1));
		gfield L;
		L.pow(x, Psub1div2);
		if (!L.len)
			return 0;
		if (L.len == 1)
			return 1;
		return -1;
	}
	[[nodiscard]] auto legendre() const -> int {
		return legendre(*this);
	}

	auto sqrt(const gfield& x) -> bool {
		if (!x.len) {
			len = 0;
			return true;
		}
#if GF_BITS == 224
#error Unsupported GF
#else
		static const gfint Padd1div4(gfint(P).add(bigint<1>(1)).rshift(2));
		switch (legendre(x)) {
		case 0:
			len = 0;
			return true;
		case -1:
			return false;
		default:
			pow(x, Padd1div4);
			return true;
		}
#endif
	}
	auto sqrt() -> bool {
		return sqrt(*this);
	}
};

struct ecjacobian {
	static const gfield B;
	static const ecjacobian base;
	static const ecjacobian origin;

	gfield x, y, z;

	ecjacobian() = default;
	ecjacobian(const gfield& x, const gfield& y) : x(x), y(y), z(bigint<1>(1)) {
	}
	ecjacobian(const gfield& x, const gfield& y, const gfield& z) : x(x), y(y), z(z) {
	}

	void mul2() {
		if (z.iszero())
			return;
		else if (y.iszero()) {
			*this = origin;
			return;
		}
		gfield a, b, c, d;
		d.sub(x, c.square(z));
		d.mul(c.add(x));
		c.mul2(d).add(d);
		z.mul(y).add(z);
		a.square(y);
		b.mul2(a);
		d.mul2(x).mul(b);
		x.square(c).sub(d).sub(d);
		a.square(b).add(a);
		y.sub(d, x).mul(c).sub(a);
	}

	void add(const ecjacobian& q) {
		if (q.z.iszero())
			return;
		else if (z.iszero()) {
			*this = q;
			return;
		}
		gfield a, b, c, d, e, f;
		a.square(z);
		b.mul(q.y, a).mul(z);
		a.mul(q.x);
		if (q.z.isone()) {
			c.add(x, a);
			d.add(y, b);
			a.sub(x, a);
			b.sub(y, b);
		} else {
			f.mul(y, e.square(q.z)).mul(q.z);
			e.mul(x);
			c.add(e, a);
			d.add(f, b);
			a.sub(e, a);
			b.sub(f, b);
		}
		if (a.iszero()) {
			if (b.iszero())
				mul2();
			else
				*this = origin;
			return;
		}
		if (!q.z.isone())
			z.mul(q.z);
		z.mul(a);
		x.square(b).sub(f.mul(c, e.square(a)));
		y.sub(f, x).sub(x).mul(b).sub(e.mul(a).mul(d)).div2();
	}

	template <int Q_DIGITS>
	void mul(const ecjacobian& p, const bigint<Q_DIGITS>& q) {
		*this = origin;
		loopirev(q.numbits()) {
			mul2();
			if (q.hasbit(i))
				add(p);
		}
	}
	template <int Q_DIGITS>
	void mul(const bigint<Q_DIGITS>& q) {
		ecjacobian tmp(*this);
		mul(tmp, q);
	}

	void normalize() {
		if (z.iszero() || z.isone())
			return;
		gfield tmp;
		z.invert();
		tmp.square(z);
		x.mul(tmp);
		y.mul(tmp).mul(z);
		z = bigint<1>(1);
	}

	auto calcy(bool ybit) -> bool {
		gfield y2, tmp;
		y2.square(x).mul(x).sub(tmp.add(x, x).add(x)).add(B);
		if (!y.sqrt(y2)) {
			y.zero();
			return false;
		}
		if (y.hasbit(0) != ybit)
			y.neg();
		return true;
	}

	void print(vector<char>& buf) {
		normalize();
		buf.add(y.hasbit(0) ? '-' : '+');
		x.printdigits(buf);
	}

	void parse(const char* s) {
		bool ybit = *s++ == '-';
		x.parse(s);
		calcy(ybit);
		z = bigint<1>(1);
	}
};

const ecjacobian ecjacobian::origin(gfield((gfield::digit)1), gfield((gfield::digit)1), gfield((gfield::digit)0));

#if GF_BITS == 192
const gfield gfield::P("fffffffffffffffffffffffffffffffeffffffffffffffff");
const gfield ecjacobian::B("64210519e59c80e70fa7e9ab72243049feb8deecc146b9b1");
const ecjacobian ecjacobian::base(gfield("188da80eb03090f67cbf20eb43a18800f4ff0afd82ff1012"),
								  gfield("07192b95ffc8da78631011ed6b24cdd573f977a11e794811"));
#elif GF_BITS == 224
const gfield gfield::P("ffffffffffffffffffffffffffffffff000000000000000000000001");
const gfield ecjacobian::B("b4050a850c04b3abf54132565044b0b7d7bfd8ba270b39432355ffb4");
const ecjacobian ecjacobian::base(gfield("b70e0cbd6bb4bf7f321390b94a03c1d356c21122343280d6115c1d21"),
								  gfield("bd376388b5f723fb4c22dfe6cd4375a05a07476444d5819985007e34"));
#elif GF_BITS == 256
const gfield gfield::P("ffffffff00000001000000000000000000000000ffffffffffffffffffffffff");
const gfield ecjacobian::B("5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b");
const ecjacobian ecjacobian::base(gfield("6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296"),
								  gfield("4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5"));
#elif GF_BITS == 384
const gfield gfield::P(
	"fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff");
const gfield ecjacobian::B(
	"b3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875ac656398d8a2ed19d2a85c8edd3ec2aef");
const ecjacobian ecjacobian::base(
	gfield("aa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7"),
	gfield("3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f"));
#elif GF_BITS == 521
const gfield gfield::P(
	"1fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
	"fffffffffffffffff");
const gfield ecjacobian::B(
	"051953eb968e1c9a1f929a21a0b68540eea2da725b99b315f3b8b489918ef109e156193951ec7e937b1652c0bd3bb1bf073573df883d2c34f1"
	"ef451fd46b503f00");
const ecjacobian ecjacobian::base(gfield("c6858e06b70404e9cd9e3ecb662395b4429c648139053fb521f828af606b4d3dbaa14b5e77efe"
										 "75928fe1dc127a2ffa8de3348b3c1856a429bf97e7e31c2e5bd66"),
								  gfield("11839296a789a3bc0045c8a5fb42c7d1bd998f54449579b446817afbd17273e662c97ee72995e"
										 "f42640c550b9013fad0761353c7086a272c24088be94769fd16650"));
#else
#error Unsupported GF
#endif

void genprivkey(const char* seed, vector<char>& privstr, vector<char>& pubstr) {
	tiger::hashval hash;
	tiger::hash((const uchar*)seed, (int)strlen(seed), hash);
	bigint<8 * sizeof(hash.bytes) / BI_DIGIT_BITS> privkey;
	memcpy(privkey.digits, hash.bytes, sizeof(hash.bytes));
	privkey.len = 8 * sizeof(hash.bytes) / BI_DIGIT_BITS;
	privkey.shrink();
	privkey.printdigits(privstr);
	privstr.add('\0');

	ecjacobian c(ecjacobian::base);
	c.mul(privkey);
	c.normalize();
	c.print(pubstr);
	pubstr.add('\0');
}

auto hashstring(const char* str, char* result, int maxlen) -> bool {
	tiger::hashval hv;
	if (maxlen < 2 * (int)sizeof(hv.bytes) + 1)
		return false;
	tiger::hash((uchar*)str, strlen(str), hv);
	loopi(sizeof(hv.bytes)) {
		uchar c = hv.bytes[i];
		*result++ = "0123456789abcdef"[c >> 4];
		*result++ = "0123456789abcdef"[c & 0xF];
	}
	*result = '\0';
	return true;
}

void answerchallenge(const char* privstr, const char* challenge, vector<char>& answerstr) {
	gfint privkey;
	privkey.parse(privstr);
	ecjacobian answer;
	answer.parse(challenge);
	answer.mul(privkey);
	answer.normalize();
	answer.x.printdigits(answerstr);
	answerstr.add('\0');
}

auto parsepubkey(const char* pubstr) -> void* {
	auto* pubkey = new ecjacobian;
	pubkey->parse(pubstr);
	return pubkey;
}

void freepubkey(void* pubkey) {
	delete (ecjacobian*)pubkey;
}

auto genchallenge(void* pubkey, const void* seed, int seedlen, vector<char>& challengestr) -> void* {
	tiger::hashval hash;
	tiger::hash((const uchar*)seed, seedlen, hash);
	gfint challenge;
	memcpy(challenge.digits, hash.bytes, sizeof(hash.bytes));
	challenge.len = 8 * sizeof(hash.bytes) / BI_DIGIT_BITS;
	challenge.shrink();

	ecjacobian answer(*(ecjacobian*)pubkey);
	answer.mul(challenge);
	answer.normalize();

	ecjacobian secret(ecjacobian::base);
	secret.mul(challenge);
	secret.normalize();

	secret.print(challengestr);
	challengestr.add('\0');

	return new gfield(answer.x);
}

void freechallenge(void* answer) {
	delete (gfint*)answer;
}

auto checkchallenge(const char* answerstr, void* correct) -> bool {
	gfint answer(answerstr);
	return answer == *(gfint*)correct;
}
