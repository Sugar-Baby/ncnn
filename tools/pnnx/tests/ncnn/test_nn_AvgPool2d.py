# Copyright 2021 Tencent
# SPDX-License-Identifier: BSD-3-Clause

import torch
import torch.nn as nn
import torch.nn.functional as F

class Model(nn.Module):
    def __init__(self):
        super(Model, self).__init__()

        self.pool_0 = nn.AvgPool2d(kernel_size=3)
        self.pool_1 = nn.AvgPool2d(kernel_size=4, stride=2, padding=2)
        self.pool_2 = nn.AvgPool2d(kernel_size=(1,3), stride=1, padding=(0,1), ceil_mode=False, count_include_pad=True)
        self.pool_3 = nn.AvgPool2d(kernel_size=(4,5), stride=(1,2), padding=(1,2), ceil_mode=True, count_include_pad=False)
        self.pool_4 = nn.AvgPool2d(kernel_size=(5,3), stride=(2,1), padding=1, ceil_mode=False, count_include_pad=True)
        self.pool_5 = nn.AvgPool2d(kernel_size=2, stride=1, padding=0, ceil_mode=True, count_include_pad=True)

    def forward(self, x):
        y = x.reshape(12, 128, 128)

        x = self.pool_0(x)
        x = self.pool_1(x)
        x = self.pool_2(x)
        x = self.pool_3(x)
        x = self.pool_4(x)
        x = self.pool_5(x)

        y = self.pool_0(y)
        y = self.pool_1(y)
        y = self.pool_2(y)
        y = self.pool_3(y)
        y = self.pool_4(y)
        y = self.pool_5(y)
        return x, y

def test():
    net = Model()
    net.eval()

    torch.manual_seed(0)
    x = torch.rand(1, 12, 128, 128)

    a = net(x)

    # export torchscript
    mod = torch.jit.trace(net, x)
    mod.save("test_nn_AvgPool2d.pt")

    # torchscript to pnnx
    import os
    os.system("../../src/pnnx test_nn_AvgPool2d.pt inputshape=[1,12,128,128]")

    # ncnn inference
    import test_nn_AvgPool2d_ncnn
    b = test_nn_AvgPool2d_ncnn.test_inference()

    for a0, b0 in zip(a, b):
        if not torch.allclose(a0, b0, 1e-4, 1e-4):
            return False
    return True

if __name__ == "__main__":
    if test():
        exit(0)
    else:
        exit(1)
