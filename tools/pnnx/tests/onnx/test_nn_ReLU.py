# Copyright 2024 Tencent
# SPDX-License-Identifier: BSD-3-Clause

import torch
import torch.nn as nn
import torch.nn.functional as F
from packaging import version

class Model(nn.Module):
    def __init__(self):
        super(Model, self).__init__()

        self.act_0 = nn.ReLU()

    def forward(self, x, y, z, w):
        x = x * 2 - 1
        y = y * 2 - 1
        z = z * 2 - 1
        w = w * 2 - 1
        x = self.act_0(x)
        y = self.act_0(y)
        z = self.act_0(z)
        w = self.act_0(w)
        return x, y, z, w

def test():
    net = Model()
    net.eval()

    torch.manual_seed(0)
    x = torch.rand(1, 12)
    y = torch.rand(1, 12, 64)
    z = torch.rand(1, 12, 24, 64)
    w = torch.rand(1, 12, 24, 32, 64)

    a = net(x, y, z, w)

    # export onnx
    torch.onnx.export(net, (x, y, z, w), "test_nn_ReLU.onnx")

    # onnx to pnnx
    import os
    os.system("../../src/pnnx test_nn_ReLU.onnx inputshape=[1,12],[1,12,64],[1,12,24,64],[1,12,24,32,64]")

    # pnnx inference
    import test_nn_ReLU_pnnx
    b = test_nn_ReLU_pnnx.test_inference()

    for a0, b0 in zip(a, b):
        if not torch.allclose(a0, b0, 1e-4, 1e-4):
            return False

    if version.parse(torch.__version__) < version.parse('2.8'):
        return True

    # export dynamo onnx
    torch.onnx.dynamo_export(net, x, y, z, w).save("test_nn_ReLU_dynamo.onnx")

    # onnx to pnnx
    os.system("../../src/pnnx test_nn_ReLU_dynamo.onnx inputshape=[1,12],[1,12,64],[1,12,24,64],[1,12,24,32,64]")

    # pnnx inference
    import test_nn_ReLU_dynamo_pnnx
    b = test_nn_ReLU_dynamo_pnnx.test_inference()

    for a0, b0 in zip(a, b):
        if not torch.allclose(a0, b0, 1e-4, 1e-4):
            return False

    return True

if __name__ == "__main__":
    if test():
        exit(0)
    else:
        exit(1)
