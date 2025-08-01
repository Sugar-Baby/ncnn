# Copyright 2021 Tencent
# SPDX-License-Identifier: BSD-3-Clause

import torch
import torch.nn as nn
import torch.nn.functional as F

class Model(nn.Module):
    def __init__(self):
        super(Model, self).__init__()

    def forward(self, x):
        y = x.reshape(12, 96, 128, 128)

        x = F.avg_pool3d(x, kernel_size=3)
        x = F.avg_pool3d(x, kernel_size=4, stride=2, padding=2)
        x = F.avg_pool3d(x, kernel_size=(1,2,3), stride=1, padding=(0,1,1), ceil_mode=False, count_include_pad=True)
        x = F.avg_pool3d(x, kernel_size=(3,4,5), stride=(1,2,2), padding=(1,1,2), ceil_mode=True, count_include_pad=False)
        x = F.avg_pool3d(x, kernel_size=(5,4,3), stride=(2,1,1), padding=1, ceil_mode=False, count_include_pad=True)
        x = F.avg_pool3d(x, kernel_size=2, stride=1, padding=0, ceil_mode=True, count_include_pad=True)
        #x = F.avg_pool3d(x, kernel_size=(5,4,4), stride=1, padding=2, ceil_mode=False, count_include_pad=False, divisor_override=77)

        y = F.avg_pool3d(y, kernel_size=3)
        y = F.avg_pool3d(y, kernel_size=4, stride=2, padding=2)
        y = F.avg_pool3d(y, kernel_size=(1,2,3), stride=1, padding=(0,1,1), ceil_mode=False, count_include_pad=True)
        y = F.avg_pool3d(y, kernel_size=(3,4,5), stride=(1,2,2), padding=(1,1,2), ceil_mode=True, count_include_pad=False)
        y = F.avg_pool3d(y, kernel_size=(5,4,3), stride=(2,1,1), padding=1, ceil_mode=False, count_include_pad=True)
        y = F.avg_pool3d(y, kernel_size=2, stride=1, padding=0, ceil_mode=True, count_include_pad=True)
        #y = F.avg_pool3d(y, kernel_size=(5,4,4), stride=1, padding=2, ceil_mode=False, count_include_pad=False, divisor_override=77)
        return x, y

def test():
    net = Model()
    net.eval()

    torch.manual_seed(0)
    x = torch.rand(1, 12, 96, 128, 128)

    a = net(x)

    # export torchscript
    mod = torch.jit.trace(net, x)
    mod.save("test_F_avg_pool3d.pt")

    # torchscript to pnnx
    import os
    os.system("../../src/pnnx test_F_avg_pool3d.pt inputshape=[1,12,96,128,128]")

    # ncnn inference
    import test_F_avg_pool3d_ncnn
    b = test_F_avg_pool3d_ncnn.test_inference()

    for a0, b0 in zip(a, b):
        if not torch.allclose(a0, b0, 1e-4, 1e-4):
            return False
    return True

if __name__ == "__main__":
    if test():
        exit(0)
    else:
        exit(1)
