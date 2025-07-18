// Copyright 2022 Tencent
// SPDX-License-Identifier: BSD-3-Clause

static void resize_bilinear_image_pack4(const Mat& src, Mat& dst, float* alpha, int* xofs, float* beta, int* yofs)
{
    int w = dst.w;
    int h = dst.h;

    // loop body
    Mat rowsbuf0(w, (size_t)4 * 4u, 4);
    Mat rowsbuf1(w, (size_t)4 * 4u, 4);
    float* rows0 = rowsbuf0;
    float* rows1 = rowsbuf1;

    int prev_sy1 = -2;

    for (int dy = 0; dy < h; dy++)
    {
        int sy = yofs[dy];

        if (sy == prev_sy1)
        {
            // reuse all rows
        }
        else if (sy == prev_sy1 + 1)
        {
            // hresize one row
            float* rows0_old = rows0;
            rows0 = rows1;
            rows1 = rows0_old;
            const float* S1 = src.row(sy + 1);

            const float* alphap = alpha;
            float* rows1p = rows1;
            int dx = 0;
            for (; dx < w; dx++)
            {
                int sx = xofs[dx] * 4;
                const float* S1p = S1 + sx;

                __m128 _a0 = _mm_set1_ps(alphap[0]);
                __m128 _a1 = _mm_set1_ps(alphap[1]);

                __m128 _S10 = _mm_load_ps(S1p);
                __m128 _S11 = _mm_load_ps(S1p + 4);
                __m128 _rows1 = _mm_mul_ps(_S10, _a0);
                _rows1 = _mm_comp_fmadd_ps(_S11, _a1, _rows1);
                _mm_store_ps(rows1p + dx * 4, _rows1);

                alphap += 2;
            }
        }
        else
        {
            // hresize two rows
            const float* S0 = src.row(sy);
            const float* S1 = src.row(sy + 1);

            const float* alphap = alpha;
            float* rows0p = rows0;
            float* rows1p = rows1;
            int dx = 0;
            for (; dx < w; dx++)
            {
                int sx = xofs[dx] * 4;
                const float* S0p = S0 + sx;
                const float* S1p = S1 + sx;

                __m128 _a0 = _mm_set1_ps(alphap[0]);
                __m128 _a1 = _mm_set1_ps(alphap[1]);

                __m128 _S00 = _mm_load_ps(S0p);
                __m128 _S01 = _mm_load_ps(S0p + 4);
                __m128 _S10 = _mm_load_ps(S1p);
                __m128 _S11 = _mm_load_ps(S1p + 4);
                __m128 _rows0 = _mm_mul_ps(_S00, _a0);
                __m128 _rows1 = _mm_mul_ps(_S10, _a0);
                _rows0 = _mm_comp_fmadd_ps(_S01, _a1, _rows0);
                _rows1 = _mm_comp_fmadd_ps(_S11, _a1, _rows1);
                _mm_store_ps(rows0p + dx * 4, _rows0);
                _mm_store_ps(rows1p + dx * 4, _rows1);

                alphap += 2;
            }
        }

        prev_sy1 = sy;

        // vresize
        __m128 _b0 = _mm_set1_ps(beta[0]);
        __m128 _b1 = _mm_set1_ps(beta[1]);

        float* rows0p = rows0;
        float* rows1p = rows1;
        float* Dp = dst.row(dy);

        for (int dx = 0; dx < w; dx++)
        {
            __m128 _rows0 = _mm_load_ps(rows0p);
            __m128 _rows1 = _mm_load_ps(rows1p);
            __m128 _Dp = _mm_mul_ps(_rows0, _b0);
            _Dp = _mm_comp_fmadd_ps(_rows1, _b1, _Dp);
            _mm_store_ps(Dp, _Dp);

            Dp += 4;
            rows0p += 4;
            rows1p += 4;
        }

        beta += 2;
    }
}
