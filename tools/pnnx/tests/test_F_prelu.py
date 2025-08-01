# Copyright 2021 Tencent
# SPDX-License-Identifier: BSD-3-Clause

import torch
import torch.nn as nn
import torch.nn.functional as F

class Model(nn.Module):
    def __init__(self):
        super(Model, self).__init__()

        self.w4 = nn.Parameter(torch.rand(16))
        self.w5 = nn.Parameter(torch.rand(2))
        self.w6 = nn.Parameter(torch.rand(3))
        self.w7 = nn.Parameter(torch.rand(1))

    def forward(self, x, y, z, w, w0, w1, w2, w3):
        x = x * 2 - 1
        y = y * 2 - 1
        z = z * 2 - 1
        w = w * 2 - 1
        x = F.prelu(x, w0)
        x = F.prelu(x, self.w4)
        y = F.prelu(y, w1)
        y = F.prelu(y, self.w5)
        z = F.prelu(z, w2)
        z = F.prelu(z, self.w6)
        w = F.prelu(w, w3)
        w = F.prelu(w, self.w7)
        return x, y, z, w

def test():
    net = Model()
    net.eval()

    torch.manual_seed(0)
    x = torch.rand(1, 16)
    y = torch.rand(12, 2, 16)
    z = torch.rand(1, 3, 12, 16)
    w = torch.rand(1, 5, 7, 9, 11)
    w0 = torch.rand(16)
    w1 = torch.rand(2)
    w2 = torch.rand(3)
    w3 = torch.rand(1)

    a0, a1, a2, a3 = net(x, y, z, w, w0, w1, w2, w3)

    # export torchscript
    mod = torch.jit.trace(net, (x, y, z, w, w0, w1, w2, w3))
    mod.save("test_F_prelu.pt")

    # torchscript to pnnx
    import os
    os.system("../src/pnnx test_F_prelu.pt inputshape=[1,16],[12,2,16],[1,3,12,16],[1,5,7,9,11],[16],[2],[3],[1]")

    # pnnx inference
    import test_F_prelu_pnnx
    b0, b1, b2, b3 = test_F_prelu_pnnx.test_inference()

    return torch.equal(a0, b0) and torch.equal(a1, b1) and torch.equal(a2, b2) and torch.equal(a3, b3)

if __name__ == "__main__":
    if test():
        exit(0)
    else:
        exit(1)
