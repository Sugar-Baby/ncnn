// Copyright 2022 Tencent
// SPDX-License-Identifier: BSD-3-Clause

static inline void interpolate_cubic(float fx, float* coeffs)
{
    const float A = -0.75f;

    float fx0 = fx + 1;
    float fx1 = fx;
    float fx2 = 1 - fx;
    // float fx3 = 2 - fx;

    coeffs[0] = A * fx0 * fx0 * fx0 - 5 * A * fx0 * fx0 + 8 * A * fx0 - 4 * A;
    coeffs[1] = (A + 2) * fx1 * fx1 * fx1 - (A + 3) * fx1 * fx1 + 1;
    coeffs[2] = (A + 2) * fx2 * fx2 * fx2 - (A + 3) * fx2 * fx2 + 1;
    coeffs[3] = 1.f - coeffs[0] - coeffs[1] - coeffs[2];
}

static void cubic_coeffs(int w, int outw, int* xofs, float* alpha, int align_corner)
{
    double scale = (double)w / outw;
    if (align_corner)
    {
        scale = (double)(w - 1) / (outw - 1);
    }

    for (int dx = 0; dx < outw; dx++)
    {
        float fx = (float)((dx + 0.5) * scale - 0.5);
        if (align_corner)
        {
            fx = (float)(dx * scale);
        }

        int sx = static_cast<int>(floor(fx));
        fx -= sx;

        interpolate_cubic(fx, alpha + dx * 4);

        if (sx <= -1)
        {
            sx = 1;
            alpha[dx * 4 + 0] = 1.f - alpha[dx * 4 + 3];
            alpha[dx * 4 + 1] = alpha[dx * 4 + 3];
            alpha[dx * 4 + 2] = 0.f;
            alpha[dx * 4 + 3] = 0.f;
        }
        if (sx == 0)
        {
            sx = 1;
            alpha[dx * 4 + 0] = alpha[dx * 4 + 0] + alpha[dx * 4 + 1];
            alpha[dx * 4 + 1] = alpha[dx * 4 + 2];
            alpha[dx * 4 + 2] = alpha[dx * 4 + 3];
            alpha[dx * 4 + 3] = 0.f;
        }
        if (sx == w - 2)
        {
            sx = w - 3;
            alpha[dx * 4 + 3] = alpha[dx * 4 + 2] + alpha[dx * 4 + 3];
            alpha[dx * 4 + 2] = alpha[dx * 4 + 1];
            alpha[dx * 4 + 1] = alpha[dx * 4 + 0];
            alpha[dx * 4 + 0] = 0.f;
        }
        if (sx >= w - 1)
        {
            sx = w - 3;
            alpha[dx * 4 + 3] = 1.f - alpha[dx * 4 + 0];
            alpha[dx * 4 + 2] = alpha[dx * 4 + 0];
            alpha[dx * 4 + 1] = 0.f;
            alpha[dx * 4 + 0] = 0.f;
        }

        xofs[dx] = sx;
    }
}

static void resize_bicubic_image(const Mat& src, Mat& dst, float* alpha, int* xofs, float* beta, int* yofs)
{
    int w = dst.w;
    int h = dst.h;

    // loop body
    Mat rowsbuf0(w);
    Mat rowsbuf1(w);
    Mat rowsbuf2(w);
    Mat rowsbuf3(w);
    float* rows0 = rowsbuf0;
    float* rows1 = rowsbuf1;
    float* rows2 = rowsbuf2;
    float* rows3 = rowsbuf3;

    int prev_sy1 = -3;

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
            rows1 = rows2;
            rows2 = rows3;
            rows3 = rows0_old;
            const float* S3 = src.row(sy + 2);

            const float* alphap = alpha;
            float* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx];
                const float* S3p = S3 + sx;

                float a0 = alphap[0];
                float a1 = alphap[1];
                float a2 = alphap[2];
                float a3 = alphap[3];
                rows3p[dx] = S3p[-1] * a0 + S3p[0] * a1 + S3p[1] * a2 + S3p[2] * a3;

                alphap += 4;
            }
        }
        else if (sy == prev_sy1 + 2)
        {
            // hresize two rows
            float* rows0_old = rows0;
            float* rows1_old = rows1;
            rows0 = rows2;
            rows1 = rows3;
            rows2 = rows0_old;
            rows3 = rows1_old;
            const float* S2 = src.row(sy + 1);
            const float* S3 = src.row(sy + 2);

            const float* alphap = alpha;
            float* rows2p = rows2;
            float* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx];
                const float* S2p = S2 + sx;
                const float* S3p = S3 + sx;

                float a0 = alphap[0];
                float a1 = alphap[1];
                float a2 = alphap[2];
                float a3 = alphap[3];
                rows2p[dx] = S2p[-1] * a0 + S2p[0] * a1 + S2p[1] * a2 + S2p[2] * a3;
                rows3p[dx] = S3p[-1] * a0 + S3p[0] * a1 + S3p[1] * a2 + S3p[2] * a3;

                alphap += 4;
            }
        }
        else if (sy == prev_sy1 + 3)
        {
            // hresize three rows
            float* rows0_old = rows0;
            float* rows1_old = rows1;
            float* rows2_old = rows2;
            rows0 = rows3;
            rows1 = rows0_old;
            rows2 = rows1_old;
            rows3 = rows2_old;
            const float* S1 = src.row(sy);
            const float* S2 = src.row(sy + 1);
            const float* S3 = src.row(sy + 2);

            const float* alphap = alpha;
            float* rows1p = rows1;
            float* rows2p = rows2;
            float* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx];
                const float* S1p = S1 + sx;
                const float* S2p = S2 + sx;
                const float* S3p = S3 + sx;

                float a0 = alphap[0];
                float a1 = alphap[1];
                float a2 = alphap[2];
                float a3 = alphap[3];
                rows1p[dx] = S1p[-1] * a0 + S1p[0] * a1 + S1p[1] * a2 + S1p[2] * a3;
                rows2p[dx] = S2p[-1] * a0 + S2p[0] * a1 + S2p[1] * a2 + S2p[2] * a3;
                rows3p[dx] = S3p[-1] * a0 + S3p[0] * a1 + S3p[1] * a2 + S3p[2] * a3;

                alphap += 4;
            }
        }
        else
        {
            // hresize four rows
            const float* S0 = src.row(sy - 1);
            const float* S1 = src.row(sy);
            const float* S2 = src.row(sy + 1);
            const float* S3 = src.row(sy + 2);

            const float* alphap = alpha;
            float* rows0p = rows0;
            float* rows1p = rows1;
            float* rows2p = rows2;
            float* rows3p = rows3;
            for (int dx = 0; dx < w; dx++)
            {
                int sx = xofs[dx];
                const float* S0p = S0 + sx;
                const float* S1p = S1 + sx;
                const float* S2p = S2 + sx;
                const float* S3p = S3 + sx;

                float a0 = alphap[0];
                float a1 = alphap[1];
                float a2 = alphap[2];
                float a3 = alphap[3];
                rows0p[dx] = S0p[-1] * a0 + S0p[0] * a1 + S0p[1] * a2 + S0p[2] * a3;
                rows1p[dx] = S1p[-1] * a0 + S1p[0] * a1 + S1p[1] * a2 + S1p[2] * a3;
                rows2p[dx] = S2p[-1] * a0 + S2p[0] * a1 + S2p[1] * a2 + S2p[2] * a3;
                rows3p[dx] = S3p[-1] * a0 + S3p[0] * a1 + S3p[1] * a2 + S3p[2] * a3;

                alphap += 4;
            }
        }

        prev_sy1 = sy;

        // vresize
        float b0 = beta[0];
        float b1 = beta[1];
        float b2 = beta[2];
        float b3 = beta[3];

        float* rows0p = rows0;
        float* rows1p = rows1;
        float* rows2p = rows2;
        float* rows3p = rows3;
        float* Dp = dst.row(dy);

        int dx = 0;
#if __SSE2__
#if __AVX__
        __m256 _b0_256 = _mm256_set1_ps(b0);
        __m256 _b1_256 = _mm256_set1_ps(b1);
        __m256 _b2_256 = _mm256_set1_ps(b2);
        __m256 _b3_256 = _mm256_set1_ps(b3);
        for (; dx + 7 < w; dx += 8)
        {
            __m256 _rows0 = _mm256_loadu_ps(rows0p);
            __m256 _rows1 = _mm256_loadu_ps(rows1p);
            __m256 _rows2 = _mm256_loadu_ps(rows2p);
            __m256 _rows3 = _mm256_loadu_ps(rows3p);
            __m256 _Dp = _mm256_mul_ps(_rows0, _b0_256);
            _Dp = _mm256_comp_fmadd_ps(_rows1, _b1_256, _Dp);
            _Dp = _mm256_comp_fmadd_ps(_rows2, _b2_256, _Dp);
            _Dp = _mm256_comp_fmadd_ps(_rows3, _b3_256, _Dp);
            _mm256_storeu_ps(Dp, _Dp);

            Dp += 8;
            rows0p += 8;
            rows1p += 8;
            rows2p += 8;
            rows3p += 8;
        }
#endif // __AVX__
        __m128 _b0_128 = _mm_set1_ps(b0);
        __m128 _b1_128 = _mm_set1_ps(b1);
        __m128 _b2_128 = _mm_set1_ps(b2);
        __m128 _b3_128 = _mm_set1_ps(b3);
        for (; dx + 3 < w; dx += 4)
        {
            __m128 _rows0 = _mm_loadu_ps(rows0p);
            __m128 _rows1 = _mm_loadu_ps(rows1p);
            __m128 _rows2 = _mm_loadu_ps(rows2p);
            __m128 _rows3 = _mm_loadu_ps(rows3p);
            __m128 _Dp = _mm_mul_ps(_rows0, _b0_128);
            _Dp = _mm_comp_fmadd_ps(_rows1, _b1_128, _Dp);
            _Dp = _mm_comp_fmadd_ps(_rows2, _b2_128, _Dp);
            _Dp = _mm_comp_fmadd_ps(_rows3, _b3_128, _Dp);
            _mm_storeu_ps(Dp, _Dp);

            Dp += 4;
            rows0p += 4;
            rows1p += 4;
            rows2p += 4;
            rows3p += 4;
        }
#endif // __SSE2__
        for (; dx < w; dx++)
        {
            *Dp++ = *rows0p++ * b0 + *rows1p++ * b1 + *rows2p++ * b2 + *rows3p++ * b3;
        }

        beta += 4;
    }
}
