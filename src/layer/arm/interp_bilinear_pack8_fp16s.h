// Copyright 2020 Tencent
// SPDX-License-Identifier: BSD-3-Clause

static void resize_bilinear_image_pack8_fp16sa(const Mat& src, Mat& dst, __fp16* alpha, int* xofs, __fp16* beta, int* yofs)
{
    int w = dst.w;
    int h = dst.h;

    // loop body
    Mat rowsbuf0(w, (size_t)8 * 2u, 8);
    Mat rowsbuf1(w, (size_t)8 * 2u, 8);
    __fp16* rows0 = rowsbuf0;
    __fp16* rows1 = rowsbuf1;

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
            __fp16* rows0_old = rows0;
            rows0 = rows1;
            rows1 = rows0_old;
            const __fp16* S1 = src.row<const __fp16>(sy + 1);

            const __fp16* alphap = alpha;
            __fp16* rows1p = rows1;
            int dx = 0;
            for (; dx < w; dx++)
            {
                int sx = xofs[dx] * 8;
                const __fp16* S1p = S1 + sx;

                float16x4_t _a01 = vld1_f16(alphap);

                float16x8_t _S10 = vld1q_f16(S1p);
                float16x8_t _S11 = vld1q_f16(S1p + 8);
                float16x8_t _rows1 = vmulq_lane_f16(_S10, _a01, 0);
                _rows1 = vfmaq_lane_f16(_rows1, _S11, _a01, 1);
                vst1q_f16(rows1p + dx * 8, _rows1);

                alphap += 2;
            }
        }
        else
        {
            // hresize two rows
            const __fp16* S0 = src.row<const __fp16>(sy);
            const __fp16* S1 = src.row<const __fp16>(sy + 1);

            const __fp16* alphap = alpha;
            __fp16* rows0p = rows0;
            __fp16* rows1p = rows1;
            int dx = 0;
            for (; dx < w; dx++)
            {
                int sx = xofs[dx] * 8;
                const __fp16* S0p = S0 + sx;
                const __fp16* S1p = S1 + sx;

                float16x4_t _a01 = vld1_f16(alphap);

                float16x8_t _S00 = vld1q_f16(S0p);
                float16x8_t _S01 = vld1q_f16(S0p + 8);
                float16x8_t _S10 = vld1q_f16(S1p);
                float16x8_t _S11 = vld1q_f16(S1p + 8);
                float16x8_t _rows0 = vmulq_lane_f16(_S00, _a01, 0);
                float16x8_t _rows1 = vmulq_lane_f16(_S10, _a01, 0);
                _rows0 = vfmaq_lane_f16(_rows0, _S01, _a01, 1);
                _rows1 = vfmaq_lane_f16(_rows1, _S11, _a01, 1);
                vst1q_f16(rows0p + dx * 8, _rows0);
                vst1q_f16(rows1p + dx * 8, _rows1);

                alphap += 2;
            }
        }

        prev_sy1 = sy;

        // vresize
        float16x4_t _b01 = vld1_f16(beta);

        __fp16* rows0p = rows0;
        __fp16* rows1p = rows1;
        __fp16* Dp = dst.row<__fp16>(dy);

        for (int dx = 0; dx < w; dx++)
        {
            float16x8_t _rows0 = vld1q_f16(rows0p);
            float16x8_t _rows1 = vld1q_f16(rows1p);
            float16x8_t _Dp = vmulq_lane_f16(_rows0, _b01, 0);
            _Dp = vfmaq_lane_f16(_Dp, _rows1, _b01, 1);
            vst1q_f16(Dp, _Dp);

            Dp += 8;
            rows0p += 8;
            rows1p += 8;
        }

        beta += 2;
    }
}
