#pragma once

namespace Engine
{
    struct Vector2
    {
        float x = 0.0f;
        float y = 0.0f;

        Vector2() = default;
        Vector2(float _x, float _y) : x(_x), y(_y) {}

        // 足し算
        Vector2 operator+(const Vector2& other) const
        {
            return Vector2(this->x + other.x, this->y + other.y);
        }
        Vector2& operator+=(const Vector2& other)
        {
            this->x += other.x;
            this->y += other.y;
            return *this;
        }

        // 引き算
        Vector2 operator-(const Vector2& other) const
        {
            return Vector2(this->x - other.x, this->y - other.y);
        }

        // スカラーとの掛け算
        Vector2 operator*(const float scalar) const
        {
            return Vector2(this->x * scalar, this->y * scalar);
        }

        bool operator==(const Vector2& other) const
        {
            return this->x == other.x && this->y == other.y;
        }



        static const Vector2 zero;
        static const Vector2 one;
        static const Vector2 minus;
    };

    inline const Vector2 Vector2::zero{ 0.0f, 0.0f };
    inline const Vector2 Vector2::one{ 1.0f, 1.0f };
    inline const Vector2 Vector2::minus{ -1.0f, -1.0f };
}
