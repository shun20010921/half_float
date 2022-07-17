#ifndef __HALF_FLOAT_H__
#define __HALF_FLOAT_H__
#include <stdio.h>
#include <stdint.h>
#include <math.h>
// Half Float Library by yaneurao
// (16-bit float)

namespace HalfFloat
{
    // IEEE 754 float 32 format is :
    //   sign(1bit) + exponent(8bits) + fraction(23bits) = 32bits
    //
    //指数部が0b11111111の場合
    //  仮数部が0の場合は符号部 * inf
    //  仮数部が0以外の場合はNan
    //指数部が0の場合
    //  仮数部も0の場合は0
    //  仮数部が0以外の場合は非正規化数

    // Our float16 format is :
    //   sign(1bit) + exponent(5bits) + fraction(10bits) = 16bits
    //
    //指数部が0b11111の場合
    //  仮数部が0の場合は符号部 * inf
    //  仮数部が0以外の場合はNan
    //指数部が0の場合
    //  仮数部も0の場合は0
    //  仮数部が0以外の場合は非正規化数
    union float32_converter
    {
        int32_t n;
        float f;
    };

    // 16-bit float
    struct float16
    {
        // --- constructors

        float16() {}
        // float16(int n) { from_float((float)n); }
        float16(int16_t n) { from_float((float)n); }
        float16(int32_t n) { from_float((float)n); }
        float16(float n) { from_float(n); }
        float16(double n) { from_float((float)n); }

        // build from a float
        void from_float(float f) { *this = to_float16(f); }

        // --- implicit converters

        operator int32_t() const { return (int32_t)to_float(*this); }
        operator float() const { return to_float(*this); }
        operator double() const { return double(to_float(*this)); }

        // --- operators

        float16 operator+=(float16 rhs)
        {
            from_float(to_float(*this) + to_float(rhs));
            return *this;
        }
        float16 operator-=(float16 rhs)
        {
            from_float(to_float(*this) - to_float(rhs));
            return *this;
        }
        float16 operator*=(float16 rhs)
        {
            from_float(to_float(*this) * to_float(rhs));
            return *this;
        }
        float16 operator/=(float16 rhs)
        {
            from_float(to_float(*this) / to_float(rhs));
            return *this;
        }
        float16 operator+(float16 rhs) const { return float16(*this) += rhs; }
        float16 operator-(float16 rhs) const { return float16(*this) -= rhs; }
        float16 operator*(float16 rhs) const { return float16(*this) *= rhs; }
        float16 operator/(float16 rhs) const { return float16(*this) /= rhs; }
        float16 operator-() const { return float16(-to_float(*this)); }
        bool operator==(float16 rhs) const { return this->v_ == rhs.v_; }
        bool operator!=(float16 rhs) const { return !(*this == rhs); }

        uint16_t v_ = 0;

    private:
        // --- entity

        // --- conversion between float and float16

        static float16 to_float16(float f)
        {
            // printf("to_float16 execution %f\n", f);
            // print_float_bit(f);
            float32_converter c;
            c.f = f;
            uint32_t n = c.n;

            uint16_t sign_bit;
            int16_t exponent;
            uint16_t fraction;

            //指数部が0b11111111の場合
            //  仮数部が0の場合は符号部 * inf
            //  仮数部が0以外の場合はNan
            //指数部が0の場合
            //  仮数部も0の場合は0
            //  仮数部が0以外の場合は非正規化数

            if (((n >> 23) & 0b11111111) == 0b11111111) //指数部が255
            {
                if ((n & 0b11111111111111111111111) == 0)
                //  仮数部が0の場合は符号部 * inf
                {
                    // printf("inf");
                    sign_bit = (n >> 31) << 15;
                    exponent = 0b11111 << 10;
                    fraction = 0;
                }
                else
                //  仮数部が0以外の場合はNan
                {
                    // printf("nan");
                    sign_bit = (n >> 31) << 15;
                    exponent = 0b11111 << 10;
                    fraction = (n >> (23 - 10)) & 0x3ff;
                }
            }
            else if (((n >> 23) & 0b11111111) == 0) //指数部が0
            {
                if ((n & 0b11111111111111111111111) == 0)
                //  仮数部も0の場合は0
                {
                    // printf("zero");
                    sign_bit = (n >> 31) << 15;
                    exponent = 0;
                    fraction = 0;
                }
                else
                //  仮数部が0以外の場合は非正規化数(アンダーフロー)
                {
                    // printf("Denormalized");
                    sign_bit = (n >> 31) << 15;
                    exponent = 0;
                    fraction = (n >> (23 - 10)) & 0x3ff;
                }
            }
            else
            {
                // printf("normalized");
                // The sign bit is MSB in common.
                sign_bit = (n >> 16) & 0x8000;
                // The exponent of IEEE 754's float 32 is biased +127 , so we change this bias into +15 and limited to 5-bit.
                exponent = (n >> 23) - 127 + 15;
                if (exponent < 0)
                {
                    exponent = 0;
                }
                else if (exponent > 31)
                {
                    exponent = 31;
                }
                exponent = (exponent & 0x1f) << 10;
                // The fraction is limited to 10-bit.
                fraction = (n >> (23 - 10)) & 0x3ff;
            }

            // printf("\n");

            float16 f_;

            f_.v_ = sign_bit | exponent | fraction;
            // putb_half_sep(f_.v_);
            return f_;
        }

        static float
        to_float(float16 v)
        {
            // Our float16 format is :
            //   sign(1bit) + exponent(5bits) + fraction(10bits) = 16bits
            //
            //指数部が0b11111の場合
            //  仮数部が0の場合は符号部 * inf
            //  仮数部が0以外の場合はNan
            //指数部が0の場合
            //  仮数部も0の場合は0
            //  仮数部が0以外の場合は非正規化数
            if (((v.v_ >> 10) & 0b11111) == 0b11111)
            //指数部が0b11111の場合
            {
                if ((v.v_ & 0b1111111111) == 0)
                //  仮数部が0の場合は符号部 * inf
                {
                    if ((v.v_ >> 15) == 1)
                    {
                        return (float)-INFINITY;
                    }
                    else
                    {
                        return (float)INFINITY;
                    }
                }
                else
                //  仮数部が0以外の場合はNan
                {
                    return NAN;
                }
            }
            else if (((v.v_ >> 10) & 0b011111) == 0)
            //指数部が0の場合
            {
                if ((v.v_ & 0b1111111111) == 0)
                //  仮数部も0の場合は0
                {
                    return (float)0;
                }
                else
                //  仮数部が0以外の場合は非正規化数
                {
                    return (float)0;
                }
            }

            uint32_t sign_bit = (v.v_ & 0x8000) << 16;
            uint32_t exponent = ((((v.v_ >> 10) & 0x1f) - 15 + 127) & 0xff) << 23;
            uint32_t fraction = (v.v_ & 0x3ff) << (23 - 10);

            float32_converter c;
            c.n = sign_bit | exponent | fraction;
            return c.f;
        }
    };
}

#endif // __HALF_FLOAT_H__
