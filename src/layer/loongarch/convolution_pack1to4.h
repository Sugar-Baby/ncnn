// Copyright 2022 yala <zhaojunchao@loongson.cn>;<junchao82@qq.com>
// SPDX-License-Identifier: BSD-3-Clause

static void convolution_pack1to4_lsx(const Mat& bottom_blob, Mat& top_blob, const Mat& weight_data_pack1ton, const Mat& bias_data, int kernel_w, int kernel_h, int dilation_w, int dilation_h, int stride_w, int stride_h, int activation_type, const Mat& activation_params, const Option& opt)
{
    int w = bottom_blob.w;
    int channels = bottom_blob.c;

    int outw = top_blob.w;
    int outh = top_blob.h;
    int outch = top_blob.c;

    const int maxk = kernel_w * kernel_h;

    // kernel offsets
    std::vector<int> _space_ofs(maxk);
    int* space_ofs = &_space_ofs[0];
    {
        int p1 = 0;
        int p2 = 0;
        int gap = w * dilation_h - kernel_w * dilation_w;
        for (int i = 0; i < kernel_h; i++)
        {
            for (int j = 0; j < kernel_w; j++)
            {
                space_ofs[p1] = p2;
                p1++;
                p2 += dilation_w;
            }
            p2 += gap;
        }
    }

    const float* bias_data_ptr = bias_data;

    // num_output
    #pragma omp parallel for num_threads(opt.num_threads)
    for (int p = 0; p < outch; p++)
    {
        float* outptr = top_blob.channel(p);

        for (int i = 0; i < outh; i++)
        {
            for (int j = 0; j < outw; j++)
            {
                __m128 _sum = (__m128)__lsx_vreplgr2vr_w(0);

                if (bias_data_ptr)
                {
                    _sum = (__m128)__lsx_vld(bias_data_ptr + p * 4, 0);
                }

                const float* kptr = (const float*)weight_data_pack1ton + maxk * channels * p * 4;

                // channels
                for (int q = 0; q < channels; q++)
                {
                    const Mat m = bottom_blob.channel(q);
                    const float* sptr = m.row(i * stride_h) + j * stride_w;

                    for (int k = 0; k < maxk; k++) // 29.23
                    {
                        __m128 _val = __lsx_vreplfr2vr_s(sptr[space_ofs[k]]);
                        __m128 _w = (__m128)__lsx_vld(kptr, 0);
                        _sum = __lsx_vfmadd_s(_w, _val, _sum);

                        kptr += 4;
                    }
                }

                _sum = activation_ps(_sum, activation_type, activation_params);

                __lsx_vst(_sum, outptr + j * 4, 0);
            }

            outptr += outw * 4;
        }
    }
}
