#pragma once

#if false
namespace Engine {


	template<typename T>
	struct Vector2T
	{
		using Vec = Vector2T<T>;

		union { T x, r = 0; };
		union { T y, g = 0; };

		Vector2T() = default;
		Vector2T(T x, T y) : x(x), y(y) {}
		Vector2T(T scalar) : x(scalar), y(scalar) {}
		Vector2T(const Vec& other) : x(other.x), y(other.y) {}

		// +
		Vec operator+(const Vec& other) const { return { x + other.x, y + other.y }; }
		Vec& operator+=(const Vec& other) { 
			x += other.x; 
			y += other.y;
			return *this;
		}
		Vec operator+(T v) const { return { x + v, y + v }; }
		Vec& operator+=(T v) {
			x += v;
			y += v;
			return *this;
		}
		// -
		Vec operator-(const Vec& other) const { return { x + other.x, y - other.y }; }
		Vec& operator-=(const Vec& other) {
			x -= other.x;
			y -= other.y;
			return *this;
		}
		Vec operator-(T v) const { return { x - v, y - v }; }
		Vec& operator-=(T v) {
			x -= v;
			y -= v;
			return *this;
		}
		// /
		Vec operator/(const Vec& other) const { return { x / other.x, y / other.y }; }
		Vec& operator/=(const Vec& other) {
			x /= other.x;
			y /= other.y;
			return *this;
		}
		Vec operator/(T v) const { return { x / v, y / v }; }
		Vec& operator/=(T v) {
			x /= v;
			y /= v;
			return *this;
		}
		// *
		Vec operator*(const Vec& other) const { return { x * other.x, y * other.y }; }
		Vec& operator*=(const Vec& other) {
			x *= other.x;
			y *= other.y;
			return *this;
		}
		Vec operator*(T v) const { return { x * v, y * v }; }
		Vec& operator*=(T v) {
			x *= v;
			y *= v;
			return *this;
		}
	};

	template<typename T>
	struct Vector3T
	{
		using Vec = Vector3T<T>;

		union { T x, r = 0; };
		union { T y, g = 0; };
		union { T z, b = 0; };

		Vector3T() = default;
		Vector3T(T x, T y, T z = 0) : x(x), y(y), z(z) {}
		Vector3T(T scalar) : x(scalar), y(scalar), z(scalar) {}
		Vector3T(const Vector2T<T>& vec2, T z = 0) : x(vec2.x), y(vec2.y), z(z) {}
		Vector3T(const Vec& other) : x(other.x), y(other.y), z(other.z) {}

		// +
		Vec operator+(const Vector2T<T>& vec2) const { return operator+(Vec(vec2, 0.0f)); }
		Vec operator+(const Vec& other) const { return { x + other.x, y + other.y, z + other.z }; }
		Vec& operator+=(const Vec& other) {
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}
		Vec operator+(T v) const { return { x + v, y + v, z + v }; }
		Vec& operator+=(T v) {
			x += v;
			y += v;
			z += v;
			return *this;
		}
		// -
		Vec operator-(const Vector2T<T>& vec2) const { return operator-(Vec(vec2, 0.0f)); }
		Vec operator-(const Vec& other) const { return { x + other.x, y - other.y, z - other.z }; }
		Vec& operator-=(const Vec& other) {
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}
		Vec operator-(T v) const { return { x - v, y - v, z - v }; }
		Vec& operator-=(T v) {
			x -= v;
			y -= v;
			z -= v;
			return *this;
		}
		// /
		Vec operator/(const Vector2T<T>& vec2) const { return operator/(Vec(vec2, 1.0f)); }
		Vec operator/(const Vec& other) const { return { x / other.x, y / other.y, z / other.z }; }
		Vec& operator/=(const Vec& other) {
			x /= other.x;
			y /= other.y;
			z /= other.z;
			return *this;
		}
		Vec operator/(T v) const { return { x / v, y / v, z / v }; }
		Vec& operator/=(T v) {
			x /= v;
			y /= v;
			z /= v;
			return *this;
		}
		// *
		Vec operator*(const Vector2T<T>& vec2) const { return operator*(Vec(vec2, 1.0f)); }
		Vec operator*(const Vec& other) const { return { x * other.x, y * other.y, z * other.z }; }
		Vec& operator*=(const Vec& other) {
			x *= other.x;
			y *= other.y;
			z *= other.z;
			return *this;
		}
		Vec operator*(T v) const { return { x * v, y * v, z * v }; }
		Vec& operator*=(T v) {
			x *= v;
			y *= v;
			z *= v;
			return *this;
		}

		bool operator==(const Vec& other) { return x == other.x && y == other.y && z == other.z; }
		bool operator!=(const Vec& other) { return !operator==(other); }

		T magnitude() const
		{
			return sqrt(x * x + y * y + z * z);
		}

		static Vec normalize(const Vec& v)
		{
			T length = v.magnitude();
			return v.operator/(length);
		}

		static Vec cross(const Vec& a, const Vec& b)
		{
			Vec result;
			result.x = a.y * b.z - a.z * b.y;
			result.y = a.z * b.x - a.x * b.z;
			result.z = a.x * b.y - a.y * b.x;
			return result;
		}
	};

	template<typename T>
	struct Vector4T
	{
		using Vec = Vector4T<T>;

		union { T x, r = 0; };
		union { T y, g = 0; };
		union { T z, b = 0; };
		union { T w, a = 0; };

		Vector4T() = default;
		Vector4T(T x, T y) : x(x), y(y) {}
		Vector4T(T x, T y, T z) : x(x), y(y), z(z) {}
		Vector4T(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
		Vector4T(T scalar) : x(scalar), y(scalar), z(scalar) {}
		Vector4T(const Vector2T<T>& vec2, T z = 0, T w = 0) : x(vec2.x), y(vec2.y), z(z), w(w) {}
		Vector4T(const Vector3T<T>& vec3, T w = 0) : x(vec3.x), y(vec3.y), z(vec3.z), w(w) {}
		Vector4T(const Vec& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}

		// +
		Vec operator+(const Vector2T<T>& vec2) const { return operator+(Vec(vec2, 0.0f, 0.0f)); }
		Vec operator+(const Vector3T<T>& vec3) const { return operator+(Vec(vec3, 0.0f)); }
		Vec operator+(const Vec& other) const { return { x + other.x, y + other.y, z + other.z }; }
		Vec& operator+=(const Vec& other) {
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}
		Vec operator+(T v) const { return { x + v, y + v, z + v }; }
		Vec& operator+=(T v) {
			x += v;
			y += v;
			z += v;
			return *this;
		}
		// -
		Vec operator-(const Vector2T<T>& vec2) const { return operator-(Vec(vec2, 0.0f, 0.0f)); }
		Vec operator-(const Vector3T<T>& vec3) const { return operator-(Vec(vec3, 0.0f)); }
		Vec operator-(const Vec& other) const { return { x + other.x, y - other.y, z - other.z }; }
		Vec& operator-=(const Vec& other) {
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}
		Vec operator-(T v) const { return { x - v, y - v, z - v }; }
		Vec& operator-=(T v) {
			x -= v;
			y -= v;
			z -= v;
			return *this;
		}
		// /
		Vec operator/(const Vector2T<T>& vec2) const { return operator-(Vec(vec2, 1.0f, 1.0f)); }
		Vec operator/(const Vector3T<T>& vec3) const { return operator/(Vec(vec3, 1.0f)); }
		Vec operator/(const Vec& other) const { return { x / other.x, y / other.y, z / other.z }; }
		Vec& operator/=(const Vec& other) {
			x /= other.x;
			y /= other.y;
			z /= other.z;
			return *this;
		}
		Vec operator/(T v) const { return { x / v, y / v, z / v }; }
		Vec& operator/=(T v) {
			x /= v;
			y /= v;
			z /= v;
			return *this;
		}
		// *
		Vec operator*(const Vector2T<T>& vec2) const { return operator*(Vec(vec2, 1.0f, 1.0f)); }
		Vec operator*(const Vector3T<T>& vec3) const { return operator*(Vec(vec3, 1.0f)); }
		Vec operator*(const Vec& other) const { return { x * other.x, y * other.y, z * other.z }; }
		Vec& operator*=(const Vec& other) {
			x *= other.x;
			y *= other.y;
			z *= other.z;
			return *this;
		}
		Vec operator*(T v) const { return { x * v, y * v, z * v }; }
		Vec& operator*=(T v) {
			x *= v;
			y *= v;
			z *= v;
			return *this;
		}

		float& operator[](size_t index) { return *(&x + index); }
		const float operator[](size_t index) const { return *(&x + index); }
	};

	template<typename T>
	struct Matrix4T
	{
		Vector4T<T> m0 = { 1, 0, 0, 0 };
		Vector4T<T> m1 = { 0, 1, 0, 0 };
		Vector4T<T> m2 = { 0, 0, 1, 0 };
		Vector4T<T> m3 = { 0, 0, 0, 1 };

		Matrix4T() = default;
		Matrix4T(T scalar) : m0(scalar), m1(scalar), m2(scalar), m3(scalar) {}
		Matrix4T(const Vector4T<T>& m0, const Vector4T<T>& m1, const Vector4T<T>& m2, const Vector4T<T>& m3)
			: m0(m0), m1(m1), m2(m2), m3(m3) {}

		static Matrix4T<T> perspective(T fovy, T aspect, T zNear, T zFar)
		{
			ASSERT(aspect != static_cast<T>(0));
			ASSERT(zFar != zNear);

			Matrix4T<T> result{ 0 };

			float yScale = 1.0f / tan(fovy * 0.5f);
			float xScale = yScale / aspect;

			result.m0.x = xScale;
			result.m1.y = yScale;
			result.m2.z = (zFar + zNear) / (zNear - zFar);
			result.m2.w = (2.0f * zFar * zNear) / (zNear - zFar);
			result.m3.z = -1.0f;
			
			return result;
		}

		static Matrix4T<T> ortho(T left, T right, T bottom, T top, T zNear, T zFar)
		{
			Matrix4T<T> result;
			result.m0.x = static_cast<T>(2) / (right - left);
			result.m1.y = static_cast<T>(2) / (top - bottom);
			result.m2.z = -static_cast<T>(2) / (zFar - zNear);

			result.m0.w = -(right + left) / (right - left);
			result.m1.w = -(top + bottom) / (top - bottom);
			result.m2.w = -(zFar + zNear) / (zFar - zNear);
			return result;
		}
		
		static Matrix4T<T> translate(const Vector3T<T>& v)
		{
			Matrix4T<T> result;
			result.m0.w = v.x;
			result.m1.w = v.y;
			result.m2.w = v.z;
			return result;
		}

		static Matrix4T<T> scale(const Vector3T<T>& v)
		{
			Matrix4T<T> result;
			result.m0.x = v.x;
			result.m1.y = v.y;
			result.m2.z = v.z;
			return result;
		}

		static Matrix4T<T> rotate(const Vector3T<T>& v)
		{
			Matrix4T<T> result;
			
			float cp = cosf(v.x);
			float sp = sinf(v.x);

			float cy = cosf(v.y);
			float sy = sinf(v.y);

			float cr = cosf(v.z);
			float sr = sinf(v.z);

			result.m0.x = cr * cy + sr * sp * sy;
			result.m0.y = sr * cp;
			result.m0.z = sr * sp * cy - cr * sy;
			result.m0.w = 0.0f;

			result.m1.x = cr * sp * sy - sr * cy;
			result.m1.y = cr * cp;
			result.m1.z = sr * sy + cr * sp * cy;
			result.m1.w = 0.0f;

			result.m2.x = cp * sy;
			result.m2.y = -sp;
			result.m2.z = cp * cy;
			result.m2.w = 0.0f;

			result.m3.x = 0.0f;
			result.m3.y = 0.0f;
			result.m3.z = 0.0f;
			result.m3.w = 1.0f;

			return result;
		}

		static Matrix4T<T> transpose(const Matrix4T<T>& matrix)
		{
			Matrix4T<T> result;
			result.m0 = { matrix.m0.x, matrix.m1.x, matrix.m2.x, matrix.m3.x };
			result.m1 = { matrix.m0.y, matrix.m1.y, matrix.m2.y, matrix.m3.y };
			result.m2 = { matrix.m0.z, matrix.m1.z, matrix.m2.z, matrix.m3.z };
			result.m3 = { matrix.m0.w, matrix.m1.w, matrix.m2.w, matrix.m3.w };

			return result;
		}

		static Matrix4T<T> inverse(const Matrix4T<T>& matrix)
		{
			const T* m = &matrix.m0.x;
			T inv[16];

			inv[0] = m[5] * m[10] * m[15] -
				m[5] * m[11] * m[14] -
				m[9] * m[6] * m[15] +
				m[9] * m[7] * m[14] +
				m[13] * m[6] * m[11] -
				m[13] * m[7] * m[10];
			inv[4] = -m[4] * m[10] * m[15] +
				m[4] * m[11] * m[14] +
				m[8] * m[6] * m[15] -
				m[8] * m[7] * m[14] -
				m[12] * m[6] * m[11] +
				m[12] * m[7] * m[10];
			inv[8] = m[4] * m[9] * m[15] -
				m[4] * m[11] * m[13] -
				m[8] * m[5] * m[15] +
				m[8] * m[7] * m[13] +
				m[12] * m[5] * m[11] -
				m[12] * m[7] * m[9];
			inv[12] = -m[4] * m[9] * m[14] +
				m[4] * m[10] * m[13] +
				m[8] * m[5] * m[14] -
				m[8] * m[6] * m[13] -
				m[12] * m[5] * m[10] +
				m[12] * m[6] * m[9];
			inv[1] = -m[1] * m[10] * m[15] +
				m[1] * m[11] * m[14] +
				m[9] * m[2] * m[15] -
				m[9] * m[3] * m[14] -
				m[13] * m[2] * m[11] +
				m[13] * m[3] * m[10];
			inv[5] = m[0] * m[10] * m[15] -
				m[0] * m[11] * m[14] -
				m[8] * m[2] * m[15] +
				m[8] * m[3] * m[14] +
				m[12] * m[2] * m[11] -
				m[12] * m[3] * m[10];
			inv[9] = -m[0] * m[9] * m[15] +
				m[0] * m[11] * m[13] +
				m[8] * m[1] * m[15] -
				m[8] * m[3] * m[13] -
				m[12] * m[1] * m[11] +
				m[12] * m[3] * m[9];
			inv[13] = m[0] * m[9] * m[14] -
				m[0] * m[10] * m[13] -
				m[8] * m[1] * m[14] +
				m[8] * m[2] * m[13] +
				m[12] * m[1] * m[10] -
				m[12] * m[2] * m[9];
			inv[2] = m[1] * m[6] * m[15] -
				m[1] * m[7] * m[14] -
				m[5] * m[2] * m[15] +
				m[5] * m[3] * m[14] +
				m[13] * m[2] * m[7] -
				m[13] * m[3] * m[6];
			inv[6] = -m[0] * m[6] * m[15] +
				m[0] * m[7] * m[14] +
				m[4] * m[2] * m[15] -
				m[4] * m[3] * m[14] -
				m[12] * m[2] * m[7] +
				m[12] * m[3] * m[6];
			inv[10] = m[0] * m[5] * m[15] -
				m[0] * m[7] * m[13] -
				m[4] * m[1] * m[15] +
				m[4] * m[3] * m[13] +
				m[12] * m[1] * m[7] -
				m[12] * m[3] * m[5];
			inv[14] = -m[0] * m[5] * m[14] +
				m[0] * m[6] * m[13] +
				m[4] * m[1] * m[14] -
				m[4] * m[2] * m[13] -
				m[12] * m[1] * m[6] +
				m[12] * m[2] * m[5];
			inv[3] = -m[1] * m[6] * m[11] +
				m[1] * m[7] * m[10] +
				m[5] * m[2] * m[11] -
				m[5] * m[3] * m[10] -
				m[9] * m[2] * m[7] +
				m[9] * m[3] * m[6];
			inv[7] = m[0] * m[6] * m[11] -
				m[0] * m[7] * m[10] -
				m[4] * m[2] * m[11] +
				m[4] * m[3] * m[10] +
				m[8] * m[2] * m[7] -
				m[8] * m[3] * m[6];
			inv[11] = -m[0] * m[5] * m[11] +
				m[0] * m[7] * m[9] +
				m[4] * m[1] * m[11] -
				m[4] * m[3] * m[9] -
				m[8] * m[1] * m[7] +
				m[8] * m[3] * m[5];
			inv[15] = m[0] * m[5] * m[10] -
				m[0] * m[6] * m[9] -
				m[4] * m[1] * m[10] +
				m[4] * m[2] * m[9] +
				m[8] * m[1] * m[6] -
				m[8] * m[2] * m[5];

			T det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
			if (det == static_cast<T>(0)) {
				return Matrix4T<T>(); // Or some identity / error signal matrix
			}

			det = static_cast<T>(1) / det;

			for (int i = 0; i < 16; ++i)
				inv[i] *= det;

			return {
				{ inv[0], inv[1], inv[2], inv[3] },
				{ inv[4], inv[5], inv[6], inv[7] },
				{ inv[8], inv[9], inv[10], inv[11] },
				{ inv[12], inv[13], inv[14], inv[15] },
			};
		}

		Matrix4T<T> operator*(const Matrix4T<T>& other) const
		{
			Matrix4T<T> result;
			result.m0 = {
				{ m0.x * other.m0.x + m0.y * other.m1.x + m0.z * other.m2.x + m0.w * other.m3.x },
				{ m0.x * other.m0.y + m0.y * other.m1.y + m0.z * other.m2.y + m0.w * other.m3.y },
				{ m0.x * other.m0.z + m0.y * other.m1.z + m0.z * other.m2.z + m0.w * other.m3.z },
				{ m0.x * other.m0.w + m0.y * other.m1.w + m0.z * other.m2.w + m0.w * other.m3.w }
			};
			result.m1 = {
				{ m1.x * other.m0.x + m1.y * other.m1.x + m1.z * other.m2.x + m1.w * other.m3.x },
				{ m1.x * other.m0.y + m1.y * other.m1.y + m1.z * other.m2.y + m1.w * other.m3.y },
				{ m1.x * other.m0.z + m1.y * other.m1.z + m1.z * other.m2.z + m1.w * other.m3.z },
				{ m1.x * other.m0.w + m1.y * other.m1.w + m1.z * other.m2.w + m1.w * other.m3.w }
			};
			result.m2 = {
				{ m2.x * other.m0.x + m2.y * other.m1.x + m2.z * other.m2.x + m2.w * other.m3.x },
				{ m2.x * other.m0.y + m2.y * other.m1.y + m2.z * other.m2.y + m2.w * other.m3.y },
				{ m2.x * other.m0.z + m2.y * other.m1.z + m2.z * other.m2.z + m2.w * other.m3.z },
				{ m2.x * other.m0.w + m2.y * other.m1.w + m2.z * other.m2.w + m2.w * other.m3.w }
			};
			result.m3 = {
				{ m3.x * other.m0.x + m3.y * other.m1.x + m3.z * other.m2.x + m3.w * other.m3.x },
				{ m3.x * other.m0.y + m3.y * other.m1.y + m3.z * other.m2.y + m3.w * other.m3.y },
				{ m3.x * other.m0.z + m3.y * other.m1.z + m3.z * other.m2.z + m3.w * other.m3.z },
				{ m3.x * other.m0.w + m3.y * other.m1.w + m3.z * other.m2.w + m3.w * other.m3.w }
			};

			return result;
		}
	};

	using Float2 = Vector2T<float>;
	using Float3 = Vector3T<float>;
	using Float4 = Vector4T<float>;

	using Int2 = Vector2T<int>;
	using Int3 = Vector3T<int>;
	using Int4 = Vector4T<int>;

	using Matrix4 = Matrix4T<float>;

}
#endif