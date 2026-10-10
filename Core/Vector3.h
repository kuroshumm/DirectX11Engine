#pragma once

namespace Engine
{
	struct Vector3
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;

		Vector3() = default;
		Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

		// 足し算
		Vector3 operator+(const Vector3& other) const
		{
			return Vector3(this->x + other.x, this->y + other.y, this->z + other.z);
		}
		Vector3& operator+=(const Vector3& other)
		{
			this->x += other.x;
			this->y += other.y;
			this->z += other.z;
			return *this;
		}

		// 引き算
		Vector3 operator-(const Vector3& other) const
		{
			return Vector3(this->x - other.x, this->y - other.y, this->z - other.z);
		}
		Vector3 operator-=(const Vector3& other)
		{
			this->x -= other.x;
			this->y -= other.y;
			this->z -= other.z;
			return *this;
		}

		// スカラーとの掛け算
		Vector3 operator*(const float scalar) const
		{
			return Vector3(this->x * scalar, this->y * scalar, this->z * scalar);
		}

		bool operator==(const Vector3& other) const
		{
			return this->x == other.x && this->y == other.y && this->z == other.z;
		}

		static const Vector3 zero;
		static const Vector3 one;
		static const Vector3 minus;

		static const Vector3 forward;
		static const Vector3 right;
	};

	inline const Vector3 Vector3::zero{ 0.0f, 0.0f, 0.0f };
	inline const Vector3 Vector3::one{ 1.0f, 1.0f, 1.0f };
	inline const Vector3 Vector3::minus{ -1.0f, -1.0f, -1.0f };

	inline const Vector3 Vector3::forward{ 0.0f, 0.0f, 1.0f };
	inline const Vector3 Vector3::right{ 1.0f, 0.0f, 0.0f };
}