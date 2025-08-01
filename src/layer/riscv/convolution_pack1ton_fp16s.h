// Copyright 2021 Tencent
// SPDX-License-Identifier: BSD-3-Clause

static void convolution_pack1ton_fp16s_rvv(const Mat& bottom_blob, Mat& top_blob, const Mat& weight_data_fp16, const Mat& bias_data, int kernel_w, int kernel_h, int dilation_w, int dilation_h, int stride_w, int stride_h, int activation_type, const Mat& activation_params, const Option& opt)
{
    const int packn = csrr_vlenb() / 2;
    const size_t vl = __riscv_vsetvl_e16m1(packn);

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
        __fp16* outptr = top_blob.channel(p);

        for (int i = 0; i < outh; i++)
        {
            for (int j = 0; j < outw; j++)
            {
                vfloat32m2_t _sum = __riscv_vfmv_v_f_f32m2(0.f, vl);

                if (bias_data_ptr)
                {
                    _sum = __riscv_vle32_v_f32m2(bias_data_ptr + p * packn, vl);
                }

                const __fp16* kptr = weight_data_fp16.channel(p);

                // channels
                for (int q = 0; q < channels; q++)
                {
                    const Mat m = bottom_blob.channel(q);
                    const __fp16* sptr = m.row<const __fp16>(i * stride_h) + j * stride_w;

                    for (int k = 0; k < maxk; k++)
                    {
                        float val = (float)sptr[space_ofs[k]];
                        vfloat16m1_t _w = __riscv_vle16_v_f16m1(kptr, vl);
                        _sum = __riscv_vfwmacc_vf_f32m2(_sum, val, _w, vl);

                        kptr += packn;
                    }
                }

                _sum = activation_ps(_sum, activation_type, activation_params, vl);

                __riscv_vse16_v_f16m1(outptr + j * packn, __riscv_vfncvt_f_f_w_f16m1(_sum, vl), vl);
            }

            outptr += outw * packn;
        }
    }
}

static void convolution_pack1ton_fp16sa_rvv(const Mat& bottom_blob, Mat& top_blob, const Mat& weight_data_fp16, const Mat& bias_data_fp16, int kernel_w, int kernel_h, int dilation_w, int dilation_h, int stride_w, int stride_h, int activation_type, const Mat& activation_params, const Option& opt)
{
    const int packn = csrr_vlenb() / 2;
    const size_t vl = __riscv_vsetvl_e16m1(packn);

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

    const __fp16* bias_data_ptr = bias_data_fp16;

    // num_output
    #pragma omp parallel for num_threads(opt.num_threads)
    for (int p = 0; p < outch; p++)
    {
        __fp16* outptr = top_blob.channel(p);

        for (int i = 0; i < outh; i++)
        {
            for (int j = 0; j < outw; j++)
            {
                vfloat16m1_t _sum = __riscv_vfmv_v_f_f16m1(0.f, vl);

                if (bias_data_ptr)
                {
                    _sum = __riscv_vle16_v_f16m1(bias_data_ptr + p * packn, vl);
                }

                const __fp16* kptr = weight_data_fp16.channel(p);

                // channels
                for (int q = 0; q < channels; q++)
                {
                    const Mat m = bottom_blob.channel(q);
                    const __fp16* sptr = m.row<const __fp16>(i * stride_h) + j * stride_w;

                    for (int k = 0; k < maxk; k++)
                    {
                        __fp16 val = sptr[space_ofs[k]];
                        vfloat16m1_t _w = __riscv_vle16_v_f16m1(kptr, vl);
                        _sum = __riscv_vfmacc_vf_f16m1(_sum, val, _w, vl);

                        kptr += packn;
                    }
                }

                _sum = activation_ps(_sum, activation_type, activation_params, vl);

                __riscv_vse16_v_f16m1(outptr + j * packn, _sum, vl);
            }

            outptr += outw * packn;
        }
    }
}
