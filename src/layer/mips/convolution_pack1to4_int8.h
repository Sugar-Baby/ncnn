// Copyright 2022 Tencent
// SPDX-License-Identifier: BSD-3-Clause

static void convolution_pack1to4_int8_msa(const Mat& bottom_blob, Mat& top_blob, const Mat& weight_data_int8, int kernel_w, int kernel_h, int dilation_w, int dilation_h, int stride_w, int stride_h, const Option& opt)
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

    // num_output
    #pragma omp parallel for num_threads(opt.num_threads)
    for (int p = 0; p < outch; p++)
    {
        int* outptr = top_blob.channel(p);

        for (int i = 0; i < outh; i++)
        {
            for (int j = 0; j < outw; j++)
            {
                v4i32 _sum = __msa_fill_w(0);

                const signed char* kptr = weight_data_int8.channel(p);

                // channels
                for (int q = 0; q < channels; q++)
                {
                    const Mat m = bottom_blob.channel(q);
                    const signed char* sptr = m.row<const signed char>(i * stride_h) + j * stride_w;

                    for (int k = 0; k < maxk; k++)
                    {
                        v8i16 _val = __msa_fill_h((short)sptr[space_ofs[k]]);

                        v16i8 _w = __msa_ld_b(kptr, 0);
                        v8i16 _w16 = (v8i16)__msa_ilvr_b(__msa_clti_s_b(_w, 0), _w);

                        v8i16 _s0 = __msa_mulv_h(_val, _w16);
                        v4i32 _s032 = (v4i32)__msa_ilvr_h(__msa_clti_s_h(_s0, 0), _s0);

                        _sum = __msa_addv_w(_sum, _s032);

                        kptr += 4;
                    }
                }

                __msa_st_w(_sum, outptr + j * 4, 0);
            }

            outptr += outw * 4;
        }
    }
}
