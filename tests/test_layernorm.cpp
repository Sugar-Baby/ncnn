// Copyright 2020 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "testutil.h"

static int test_layernorm(const ncnn::Mat& a, int affine_size, float eps, int affine)
{
    ncnn::ParamDict pd;
    pd.set(0, affine_size);
    pd.set(1, eps);
    pd.set(2, affine);

    std::vector<ncnn::Mat> weights(2);
    weights[0] = RandomMat(affine_size);
    weights[1] = RandomMat(affine_size);

    int ret = test_layer("LayerNorm", pd, weights, a);
    if (ret != 0)
    {
        fprintf(stderr, "test_layernorm failed a.dims=%d a=(%d %d %d) affine_size=%d eps=%f affine=%d\n", a.dims, a.w, a.h, a.c, affine_size, eps, affine);
    }

    return ret;
}

static int test_layernorm_0()
{
    return 0
           || test_layernorm(RandomMat(6, 4, 2), 6, 0.01f, 0)
           || test_layernorm(RandomMat(4, 5, 6), 4, 0.01f, 0)
           || test_layernorm(RandomMat(3, 3, 8), 3, 0.002f, 0)
           || test_layernorm(RandomMat(5, 6, 12), 5, 0.02f, 0)
           || test_layernorm(RandomMat(4, 7, 16), 4, 0.02f, 0)
           || test_layernorm(RandomMat(6, 7, 24), 6, 0.001f, 0)
           || test_layernorm(RandomMat(5, 8, 32), 5, 0.001f, 0)
           || test_layernorm(RandomMat(6, 4, 2), 6, 0.01f, 1)
           || test_layernorm(RandomMat(4, 5, 6), 4, 0.01f, 1)
           || test_layernorm(RandomMat(3, 3, 8), 3, 0.002f, 1)
           || test_layernorm(RandomMat(5, 6, 12), 5, 0.02f, 1)
           || test_layernorm(RandomMat(4, 7, 16), 4, 0.02f, 1)
           || test_layernorm(RandomMat(6, 7, 24), 6, 0.001f, 1)
           || test_layernorm(RandomMat(5, 8, 32), 5, 0.001f, 1);
}

static int test_layernorm_1()
{
    return 0
           || test_layernorm(RandomMat(6, 4, 2), 24, 0.01f, 0)
           || test_layernorm(RandomMat(4, 5, 6), 20, 0.01f, 0)
           || test_layernorm(RandomMat(3, 3, 8), 9, 0.002f, 0)
           || test_layernorm(RandomMat(5, 6, 12), 30, 0.02f, 0)
           || test_layernorm(RandomMat(4, 7, 16), 28, 0.02f, 0)
           || test_layernorm(RandomMat(6, 7, 24), 42, 0.001f, 0)
           || test_layernorm(RandomMat(5, 8, 32), 40, 0.001f, 0)
           || test_layernorm(RandomMat(6, 4, 2), 24, 0.01f, 1)
           || test_layernorm(RandomMat(4, 5, 6), 20, 0.01f, 1)
           || test_layernorm(RandomMat(3, 3, 8), 9, 0.002f, 1)
           || test_layernorm(RandomMat(5, 6, 12), 30, 0.02f, 1)
           || test_layernorm(RandomMat(4, 7, 16), 28, 0.02f, 1)
           || test_layernorm(RandomMat(6, 7, 24), 42, 0.001f, 1)
           || test_layernorm(RandomMat(5, 8, 32), 40, 0.001f, 1);
}

static int test_layernorm_2()
{
    return 0
           || test_layernorm(RandomMat(4, 2), 4, 0.01f, 0)
           || test_layernorm(RandomMat(5, 6), 5, 0.01f, 0)
           || test_layernorm(RandomMat(3, 8), 3, 0.002f, 0)
           || test_layernorm(RandomMat(6, 12), 6, 0.02f, 0)
           || test_layernorm(RandomMat(4, 16), 4, 0.02f, 0)
           || test_layernorm(RandomMat(7, 24), 7, 0.001f, 0)
           || test_layernorm(RandomMat(8, 32), 8, 0.001f, 0)
           || test_layernorm(RandomMat(4, 2), 4, 0.01f, 1)
           || test_layernorm(RandomMat(5, 6), 5, 0.01f, 1)
           || test_layernorm(RandomMat(3, 8), 3, 0.002f, 1)
           || test_layernorm(RandomMat(6, 12), 6, 0.02f, 1)
           || test_layernorm(RandomMat(4, 16), 4, 0.02f, 1)
           || test_layernorm(RandomMat(7, 24), 7, 0.001f, 1)
           || test_layernorm(RandomMat(8, 32), 8, 0.001f, 1);
}

static int test_layernorm_3()
{
    return 0
           || test_layernorm(RandomMat(2), 2, 0.01f, 0)
           || test_layernorm(RandomMat(6), 6, 0.01f, 0)
           || test_layernorm(RandomMat(8), 8, 0.002f, 0)
           || test_layernorm(RandomMat(12), 12, 0.02f, 0)
           || test_layernorm(RandomMat(16), 16, 0.02f, 0)
           || test_layernorm(RandomMat(24), 24, 0.001f, 0)
           || test_layernorm(RandomMat(32), 32, 0.001f, 0)
           || test_layernorm(RandomMat(2), 2, 0.01f, 1)
           || test_layernorm(RandomMat(6), 6, 0.01f, 1)
           || test_layernorm(RandomMat(8), 8, 0.002f, 1)
           || test_layernorm(RandomMat(12), 12, 0.02f, 1)
           || test_layernorm(RandomMat(16), 16, 0.02f, 1)
           || test_layernorm(RandomMat(24), 24, 0.001f, 1)
           || test_layernorm(RandomMat(32), 32, 0.001f, 1);
}

int main()
{
    SRAND(7767517);

    return 0
           || test_layernorm_0()
           || test_layernorm_1()
           || test_layernorm_2()
           || test_layernorm_3();
}
