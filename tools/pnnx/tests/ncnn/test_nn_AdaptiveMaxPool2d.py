# Copyright 2021 Tencent
# SPDX-License-Identifier: BSD-3-Clause

import torch
import torch.nn as nn
import torch.nn.functional as F

class Model(nn.Module):
    def __init__(self):
        super(Model, self).__init__()

        self.pool_0 = nn.AdaptiveMaxPool2d(output_size=(7,6))
        self.pool_1 = nn.AdaptiveMaxPool2d(output_size=1)

    def forward(self, x):
        x = self.pool_0(x)
        x = self.pool_1(x)
        return x

def test():
    net = Model()
    net.eval()

    torch.manual_seed(0)
    x = torch.rand(1, 128, 13, 13)

    a = net(x)

    # export torchscript
    mod = torch.jit.trace(net, x)
    mod.save("test_nn_AdaptiveMaxPool2d.pt")

    # torchscript to pnnx
    import os
    os.system("../../src/pnnx test_nn_AdaptiveMaxPool2d.pt inputshape=[1,128,13,13]")

    # ncnn inference
    import test_nn_AdaptiveMaxPool2d_ncnn
    b = test_nn_AdaptiveMaxPool2d_ncnn.test_inference()

    b = b.reshape_as(a)

    return torch.allclose(a, b, 1e-4, 1e-4)

if __name__ == "__main__":
    if test():
        exit(0)
    else:
        exit(1)
