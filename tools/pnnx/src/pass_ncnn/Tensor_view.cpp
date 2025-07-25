// Copyright 2021 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "pass_ncnn.h"

namespace pnnx {

namespace ncnn {

class Tensor_view : public GraphRewriterPass
{
public:
    const char* match_pattern_graph() const
    {
        return R"PNNXIR(7767517
3 2
pnnx.Input              input       0 1 input
Tensor.view             op_0        1 1 input out shape=%shape
pnnx.Output             output      1 0 out
)PNNXIR";
    }

    const char* type_str() const
    {
        return "Reshape";
    }

    const char* name_str() const
    {
        return "view";
    }

    void write(Operator* op, const std::map<std::string, Parameter>& captured_params) const
    {
        const std::vector<int>& shape = captured_params.at("shape").ai;

        const int batch_index = op->outputs[0]->params["__batch_index"].i;

        if (batch_index != 0 && batch_index != 233)
        {
            if (op->outputs[0]->shape.empty() || op->outputs[0]->shape[batch_index] != 1)
                fprintf(stderr, "reshape tensor with batch index %d is not supported yet!\n", batch_index);
        }

        // drop shape batch index
        std::vector<int> new_shape;
        for (int i = 0; i < (int)shape.size(); i++)
        {
            if (i == batch_index && shape[i] == 1)
                continue;

            new_shape.push_back(shape[i]);
        }

        if (new_shape.size() == 5 && batch_index == 233)
        {
            if (new_shape[0] == 1)
            {
                fprintf(stderr, "assume reshape 5-rank tensor has batch_index 0\n");
                new_shape.erase(new_shape.begin());
            }
        }

        const int shape_rank = (int)new_shape.size();

        if (shape_rank > 5)
        {
            fprintf(stderr, "reshape to %d-rank tensor is not supported yet!\n", shape_rank);
            return;
        }

        if (shape_rank == 1)
        {
            op->params["0"] = new_shape[0];
        }
        if (shape_rank == 2)
        {
            op->params["0"] = new_shape[1];
            op->params["1"] = new_shape[0];
        }
        if (shape_rank == 3)
        {
            op->params["0"] = new_shape[2];
            op->params["1"] = new_shape[1];
            op->params["2"] = new_shape[0];
        }
        if (shape_rank == 4)
        {
            op->params["0"] = new_shape[3];
            op->params["1"] = new_shape[2];
            op->params["11"] = new_shape[1];
            op->params["2"] = new_shape[0];
        }
    }
};

REGISTER_GLOBAL_PNNX_NCNN_GRAPH_REWRITER_PASS(Tensor_view, 20)

} // namespace ncnn

} // namespace pnnx
