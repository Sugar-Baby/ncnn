// Copyright 2024 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "innerproduct_riscv.h"

#if __riscv_vector
#include <riscv_vector.h>
#endif // __riscv_vector
#include "riscv_activation.h"
#include "riscv_usability.h"

namespace ncnn {

#if NCNN_ZFH
int InnerProduct_riscv::create_pipeline_fp16s(const Option& opt)
{
#if __riscv_zvfh
    const int packn = csrr_vlenb() / 2;
#endif // __riscv_zvfh

    const int num_input = weight_data_size / num_output;

    int out_elempack = 1;

#if __riscv_zvfh
    if (opt.use_packing_layout)
    {
        out_elempack = num_output % packn == 0 ? packn : 1;
    }
#endif // __riscv_zvfh

    // src = inch-outch
    // dst = pb-inch-outch/pb
    {
        Mat weight_data_r2 = weight_data.reshape(num_input, num_output);

        weight_data_tm.create(num_input, num_output / out_elempack, (size_t)2u * out_elempack, out_elempack);

        for (int q = 0; q + (out_elempack - 1) < num_output; q += out_elempack)
        {
            __fp16* g0 = weight_data_tm.row<__fp16>(q / out_elempack);

            for (int p = 0; p < num_input; p++)
            {
                for (int j = 0; j < out_elempack; j++)
                {
                    *g0++ = (__fp16)(weight_data_r2.row(q + j)[p]);
                }
            }
        }
    }

    ncnn::cast_float32_to_float16(bias_data, bias_data_fp16, opt);

    if (opt.lightmode)
        weight_data.release();

    return 0;
}

int InnerProduct_riscv::forward_fp16s(const Mat& bottom_blob, Mat& top_blob, const Option& opt) const
{
#if __riscv_zvfh
    const int packn = csrr_vlenb() / 2;
#endif // __riscv_zvfh

    const int num_input = weight_data_size / num_output;

    if (bottom_blob.dims == 2 && bottom_blob.w == num_input)
    {
        // gemm
        int h = bottom_blob.h;
        size_t elemsize = bottom_blob.elemsize;
        int elempack = bottom_blob.elempack;

        top_blob.create(num_output, h, elemsize, elempack, opt.blob_allocator);
        if (top_blob.empty())
            return -100;

        int num_output_elempack = 1;
#if __riscv_zvfh
        if (opt.use_packing_layout)
        {
            num_output_elempack = num_output % packn == 0 ? packn : 1;
        }
#endif // __riscv_zvfh

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int j = 0; j < h; j++)
        {
#if __riscv_zvfh
            if (elempack == packn && num_output_elempack == packn)
            {
                const size_t vl = __riscv_vsetvl_e16m1(packn);

                __fp16* outptr = top_blob.row<__fp16>(j);

                for (int p = 0; p < num_output / num_output_elempack; p++)
                {
                    for (int l = 0; l < packn; l++)
                    {
                        const __fp16* kptr = (const __fp16*)weight_data_tm + num_input * p * packn + l;
                        const __fp16* m = bottom_blob.row<const __fp16>(j);

                        vfloat32m2_t _sum = __riscv_vfmv_v_f_f32m2(0.f, vl);

                        if (bias_term)
                        {
                            _sum = __riscv_vfmv_v_f_f32m2(bias_data[p * packn + l], vl);
                        }

                        int n = num_input;
                        while (n > 0)
                        {
                            vfloat32m2_t _val = __riscv_vfwcvt_f_f_v_f32m2(__riscv_vle16_v_f16m1(m, vl), vl);

                            _sum = __riscv_vfmacc_vf_f32m2(_sum, *kptr, _val, vl);

                            m += packn;
                            kptr += packn;
                            n -= 1;
                        }

                        _sum = activation_ps(_sum, activation_type, activation_params, vl);

                        __riscv_vse16_v_f16m1(outptr, __riscv_vfncvt_f_f_w_f16m1(_sum, vl), vl);
                        outptr += packn;
                    }
                }
            }

            if (elempack == 1 && num_output_elempack == packn)
            {
                const size_t vl = __riscv_vsetvl_e16m1(packn);

                __fp16* outptr = top_blob.row<__fp16>(j);

                for (int p = 0; p < num_output / num_output_elempack; p++)
                {
                    const __fp16* kptr = (const __fp16*)weight_data_tm + num_input * p * packn;
                    const __fp16* m = bottom_blob.row<const __fp16>(j);

                    vfloat32m2_t _sum = __riscv_vfmv_v_f_f32m2(0.f, vl);

                    if (bias_term)
                    {
                        _sum = __riscv_vle32_v_f32m2((const float*)bias_data + p * packn, vl);
                    }

                    int n = num_input;
                    while (n > 0)
                    {
                        vfloat32m2_t _w = __riscv_vfwcvt_f_f_v_f32m2(__riscv_vle16_v_f16m1(kptr, vl), vl);

                        _sum = __riscv_vfmacc_vf_f32m2(_sum, *m, _w, vl);

                        m += 1;
                        kptr += packn;
                        n -= 1;
                    }

                    _sum = activation_ps(_sum, activation_type, activation_params, vl);

                    __riscv_vse16_v_f16m1(outptr, __riscv_vfncvt_f_f_w_f16m1(_sum, vl), vl);
                    outptr += packn;
                }
            }

            if (elempack == packn && num_output_elempack == 1)
            {
                const size_t vl = __riscv_vsetvl_e16m1(packn);

                __fp16* outptr = top_blob.row<__fp16>(j);

                for (int p = 0; p < num_output; p++)
                {
                    const __fp16* kptr = (const __fp16*)weight_data_tm + num_input * p;
                    const __fp16* m = bottom_blob.row<const __fp16>(j);

                    vfloat32m2_t _sum = __riscv_vfmv_v_f_f32m2(0.f, vl);

                    if (bias_term)
                    {
                        _sum = __riscv_vfmv_v_f_f32m2(bias_data[p], vl);
                    }

                    int n = num_input;
                    while (n > 0)
                    {
                        vfloat32m2_t _val = __riscv_vfwcvt_f_f_v_f32m2(__riscv_vle16_v_f16m1(m, vl), vl);

                        _sum = __riscv_vfmacc_vf_f32m2(_sum, *kptr, _val, vl);

                        m += packn;
                        kptr += 1;
                        n -= 1;
                    }

                    _sum = activation_ps(_sum, activation_type, activation_params, vl);

                    __riscv_vse16_v_f16m1(outptr, __riscv_vfncvt_f_f_w_f16m1(_sum, vl), vl);
                    outptr += packn;
                }
            }
#endif // __riscv_zvfh

            if (elempack == 1 && num_output_elempack == 1)
            {
                __fp16* outptr = top_blob.row<__fp16>(j);

                for (int p = 0; p < num_output; p++)
                {
                    const __fp16* kptr = (const __fp16*)weight_data_tm + num_input * p;
                    const __fp16* m = bottom_blob.row<const __fp16>(j);

                    float sum = 0.f;

                    if (bias_term)
                    {
                        sum = bias_data[p];
                    }

                    for (int i = 0; i < num_input; i++)
                    {
                        sum += (float)m[i] * (float)kptr[i];
                    }

                    sum = activation_ss(sum, activation_type, activation_params);

                    outptr[0] = (__fp16)sum;
                    outptr += 1;
                }
            }
        }

        return 0;
    }

    // flatten
    Mat bottom_blob_flattened = bottom_blob;
    if (bottom_blob.dims != 1)
    {
        Option opt_flatten = opt;
        opt_flatten.blob_allocator = opt.workspace_allocator;

        flatten->forward(bottom_blob, bottom_blob_flattened, opt_flatten);
    }

    size_t elemsize = bottom_blob_flattened.elemsize;
    int elempack = bottom_blob_flattened.elempack;

    int out_elempack = 1;
#if __riscv_zvfh
    if (opt.use_packing_layout)
    {
        out_elempack = num_output % packn == 0 ? packn : 1;
    }
#endif // __riscv_zvfh
    size_t out_elemsize = elemsize / elempack * out_elempack;

    top_blob.create(num_output / out_elempack, out_elemsize, out_elempack, opt.blob_allocator);
    if (top_blob.empty())
        return -100;

#if __riscv_zvfh
    if (out_elempack == packn)
    {
        // num_output
        #pragma omp parallel for num_threads(opt.num_threads)
        for (int p = 0; p < num_output / out_elempack; p++)
        {
            const size_t vl = __riscv_vsetvl_e16m1(packn);
            vfloat32m2_t _sum = __riscv_vfmv_v_f_f32m2(0.f, vl);

            if (bias_term)
            {
                _sum = __riscv_vle32_v_f32m2((const float*)bias_data + p * packn, vl);
            }

            const __fp16* kptr = weight_data_tm.row<const __fp16>(p);

            const __fp16* sptr = bottom_blob_flattened;

            int n = num_input;
            while (n > 0)
            {
                vfloat32m2_t _w = __riscv_vfwcvt_f_f_v_f32m2(__riscv_vle16_v_f16m1(kptr, vl), vl);

                _sum = __riscv_vfmacc_vf_f32m2(_sum, (float)(*sptr), _w, vl);

                sptr += 1;
                kptr += packn;
                n -= 1;
            }

            _sum = activation_ps(_sum, activation_type, activation_params, vl);

            __fp16* outptr = (__fp16*)top_blob;
            __riscv_vse16_v_f16m1(outptr + p * packn, __riscv_vfncvt_f_f_w_f16m1(_sum, vl), vl);
        }
    }
#endif // __riscv_zvfh

    if (out_elempack == 1)
    {
        // num_output
        #pragma omp parallel for num_threads(opt.num_threads)
        for (int p = 0; p < num_output; p++)
        {
            float sum = 0.f;

            if (bias_term)
                sum = bias_data[p];

            const __fp16* kptr = weight_data_tm.row<__fp16>(p);

            const __fp16* sptr = bottom_blob_flattened;

            int i = 0;
            for (; i < num_input; i++)
            {
                float v = (float)(*sptr);
                float k = (float)(*kptr);

                sum += v * k;

                sptr++;
                kptr++;
            }

            sum = activation_ss(sum, activation_type, activation_params);

            __fp16* outptr = (__fp16*)top_blob;
            outptr[p] = (__fp16)sum;
        }
    }

    return 0;
}

int InnerProduct_riscv::forward_fp16sa(const Mat& bottom_blob, Mat& top_blob, const Option& opt) const
{
#if __riscv_zvfh
    const int packn = csrr_vlenb() / 2;
#endif // __riscv_zvfh

    const int num_input = weight_data_size / num_output;

    if (bottom_blob.dims == 2 && bottom_blob.w == num_input)
    {
        // gemm
        int h = bottom_blob.h;
        size_t elemsize = bottom_blob.elemsize;
        int elempack = bottom_blob.elempack;

        top_blob.create(num_output, h, elemsize, elempack, opt.blob_allocator);
        if (top_blob.empty())
            return -100;

        int num_output_elempack = 1;
#if __riscv_zvfh
        if (opt.use_packing_layout)
        {
            num_output_elempack = num_output % packn == 0 ? packn : 1;
        }
#endif // __riscv_zvfh

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int j = 0; j < h; j++)
        {
#if __riscv_zvfh
            if (elempack == packn && num_output_elempack == packn)
            {
                const size_t vl = __riscv_vsetvl_e16m1(packn);

                __fp16* outptr = top_blob.row<__fp16>(j);

                for (int p = 0; p < num_output / num_output_elempack; p++)
                {
                    for (int l = 0; l < packn; l++)
                    {
                        const __fp16* kptr = (const __fp16*)weight_data_tm + num_input * p * packn + l;
                        const __fp16* m = bottom_blob.row<const __fp16>(j);

                        vfloat16m1_t _sum = __riscv_vfmv_v_f_f16m1((__fp16)0.f, vl);

                        if (bias_term)
                        {
                            _sum = __riscv_vfmv_v_f_f16m1(((const __fp16*)bias_data_fp16)[p * packn + l], vl);
                        }

                        int n = num_input;
                        while (n > 0)
                        {
                            vfloat16m1_t _val = __riscv_vle16_v_f16m1(m, vl);

                            _sum = __riscv_vfmacc_vf_f16m1(_sum, *kptr, _val, vl);

                            m += packn;
                            kptr += packn;
                            n -= 1;
                        }

                        _sum = activation_ps(_sum, activation_type, activation_params, vl);

                        __riscv_vse16_v_f16m1(outptr, _sum, vl);
                        outptr += packn;
                    }
                }
            }

            if (elempack == 1 && num_output_elempack == packn)
            {
                const size_t vl = __riscv_vsetvl_e16m1(packn);

                __fp16* outptr = top_blob.row<__fp16>(j);

                for (int p = 0; p < num_output / num_output_elempack; p++)
                {
                    const __fp16* kptr = (const __fp16*)weight_data_tm + num_input * p * packn;
                    const __fp16* m = bottom_blob.row<const __fp16>(j);

                    vfloat16m1_t _sum = __riscv_vfmv_v_f_f16m1(0.f, vl);

                    if (bias_term)
                    {
                        _sum = __riscv_vle16_v_f16m1((const __fp16*)bias_data_fp16 + p * packn, vl);
                    }

                    int n = num_input;
                    while (n > 0)
                    {
                        vfloat16m1_t _w = __riscv_vle16_v_f16m1(kptr, vl);

                        _sum = __riscv_vfmacc_vf_f16m1(_sum, *m, _w, vl);

                        m += 1;
                        kptr += packn;
                        n -= 1;
                    }

                    _sum = activation_ps(_sum, activation_type, activation_params, vl);

                    __riscv_vse16_v_f16m1(outptr, _sum, vl);
                    outptr += packn;
                }
            }

            if (elempack == packn && num_output_elempack == 1)
            {
                const size_t vl = __riscv_vsetvl_e16m1(packn);

                __fp16* outptr = top_blob.row<__fp16>(j);

                for (int p = 0; p < num_output; p++)
                {
                    const __fp16* kptr = (const __fp16*)weight_data_tm + num_input * p;
                    const __fp16* m = bottom_blob.row<const __fp16>(j);

                    vfloat16m1_t _sum = __riscv_vfmv_v_f_f16m1(0.f, vl);

                    if (bias_term)
                    {
                        _sum = __riscv_vfmv_v_f_f16m1(((const __fp16*)bias_data_fp16)[p], vl);
                    }

                    int n = num_input;
                    while (n > 0)
                    {
                        vfloat16m1_t _val = __riscv_vle16_v_f16m1(m, vl);

                        _sum = __riscv_vfmacc_vf_f16m1(_sum, *kptr, _val, vl);

                        m += packn;
                        kptr += 1;
                        n -= 1;
                    }

                    _sum = activation_ps(_sum, activation_type, activation_params, vl);

                    __riscv_vse16_v_f16m1(outptr, _sum, vl);
                    outptr += packn;
                }
            }
#endif // __riscv_zvfh

            if (elempack == 1 && num_output_elempack == 1)
            {
                __fp16* outptr = top_blob.row<__fp16>(j);

                for (int p = 0; p < num_output; p++)
                {
                    const __fp16* kptr = (const __fp16*)weight_data_tm + num_input * p;
                    const __fp16* m = bottom_blob.row<const __fp16>(j);

                    float sum = 0.f;

                    if (bias_term)
                    {
                        sum = bias_data[p];
                    }

                    for (int i = 0; i < num_input; i++)
                    {
                        sum += (float)(m[i] * kptr[i]);
                    }

                    sum = activation_ss(sum, activation_type, activation_params);

                    outptr[0] = (__fp16)sum;
                    outptr += 1;
                }
            }
        }

        return 0;
    }

    // flatten
    Mat bottom_blob_flattened = bottom_blob;
    if (bottom_blob.dims != 1)
    {
        Option opt_flatten = opt;
        opt_flatten.blob_allocator = opt.workspace_allocator;

        flatten->forward(bottom_blob, bottom_blob_flattened, opt_flatten);
    }

    size_t elemsize = bottom_blob_flattened.elemsize;
    int elempack = bottom_blob_flattened.elempack;

    int out_elempack = 1;
#if __riscv_zvfh
    if (opt.use_packing_layout)
    {
        out_elempack = num_output % packn == 0 ? packn : 1;
    }
#endif // __riscv_zvfh
    size_t out_elemsize = elemsize / elempack * out_elempack;

    top_blob.create(num_output / out_elempack, out_elemsize, out_elempack, opt.blob_allocator);
    if (top_blob.empty())
        return -100;

#if __riscv_zvfh
    if (out_elempack == packn)
    {
        // num_output
        #pragma omp parallel for num_threads(opt.num_threads)
        for (int p = 0; p < num_output / out_elempack; p++)
        {
            const size_t vl = __riscv_vsetvl_e16m1(packn);
            vfloat16m1_t _sum = __riscv_vfmv_v_f_f16m1(0.f, vl);

            if (bias_term)
            {
                _sum = __riscv_vle16_v_f16m1((const __fp16*)bias_data_fp16 + p * packn, vl);
            }

            const __fp16* kptr = weight_data_tm.row<const __fp16>(p);

            const __fp16* sptr = bottom_blob_flattened;

            int n = num_input;
            while (n > 0)
            {
                vfloat16m1_t _w = __riscv_vle16_v_f16m1(kptr, vl);

                _sum = __riscv_vfmacc_vf_f16m1(_sum, *sptr, _w, vl);

                sptr += 1;
                kptr += packn;
                n -= 1;
            }

            _sum = activation_ps(_sum, activation_type, activation_params, vl);

            __fp16* outptr = (__fp16*)top_blob;
            __riscv_vse16_v_f16m1(outptr + p * packn, _sum, vl);
        }
    }
#endif // __riscv_zvfh

    if (out_elempack == 1)
    {
        // num_output
        #pragma omp parallel for num_threads(opt.num_threads)
        for (int p = 0; p < num_output; p++)
        {
            float sum = 0.f;

            if (bias_term)
                sum = bias_data[p];

            const __fp16* kptr = weight_data_tm.row<__fp16>(p);

            const __fp16* sptr = bottom_blob_flattened;

            int i = 0;
            for (; i < num_input; i++)
            {
                __fp16 v = *sptr;
                __fp16 k = *kptr;

                sum += (float)(v * k);

                sptr++;
                kptr++;
            }

            sum = activation_ss(sum, activation_type, activation_params);

            __fp16* outptr = (__fp16*)top_blob;
            outptr[p] = (__fp16)sum;
        }
    }

    return 0;
}
#endif // NCNN_ZFH

} // namespace ncnn
