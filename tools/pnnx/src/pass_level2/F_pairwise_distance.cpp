// Copyright 2023 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "pass_level2.h"

namespace pnnx {

class F_pairwise_distance : public GraphRewriterPass
{
public:
    const char* match_pattern_graph() const
    {
        return R"PNNXIR(7767517
7 6
pnnx.Input              input_0         0 1 x1
pnnx.Input              input_1         0 1 x2
prim::Constant          op_0            0 1 p value=%p
prim::Constant          op_1            0 1 eps value=%eps
prim::Constant          op_2            0 1 keepdim value=%keepdim
aten::pairwise_distance op_3            5 1 x1 x2 p eps keepdim out
pnnx.Output             output          1 0 out
)PNNXIR";
    }

    const char* type_str() const
    {
        return "F.pairwise_distance";
    }
};

REGISTER_GLOBAL_PNNX_GRAPH_REWRITER_PASS(F_pairwise_distance, 110)

} // namespace pnnx
