// Copyright 2020 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef NCNN_REWRITER_TD
#define NCNN_REWRITER_TD

include "tf_ops.td"
include "ncnn_ops.td"

def get_attr_f : NativeCodeCall<"$0.getValue<FloatAttr>(0)">;

def OneElementAttrPred : CPred<"$_self.cast<ElementsAttr>().getType().getNumElements() == 1">;

def OneElementAttr : ElementsAttrBase<And<[ElementsAttr.predicate, OneElementAttrPred]>, "Scalar ElementsAttr">;

def EqualOperands : Constraint<CPred<"$0 == $1">>;

def FuseBinaryOpPattern0 : Pat<
    (TF_MulOp
        $x,
        (TF_ConstOp OneElementAttr:$b)
    ),

    (NCNN_BinaryOpOp $x, ConstantAttr<I32Attr, "2">, ConstantAttr<I32Attr, "1">, (get_attr_f $b))
>;

def FuseBinaryOpPattern1 : Pat<
    (TF_AddV2Op
        $x,
        (TF_ConstOp OneElementAttr:$b)
    ),

    (NCNN_BinaryOpOp $x, ConstantAttr<I32Attr, "0">, ConstantAttr<I32Attr, "1">, (get_attr_f $b))
>;

def FuseKerasConv2DOpPattern : Pat<
    (TF_BiasAddOp
        (TF_Conv2DOp $x, $weight, $strides, $use_cudnn_on_gpu, $padding, $explicit_paddings, $data_format, $dilations),
        $bias,
        $data_format_
    ),

    (NCNN_KerasConv2DOp $x, $weight, $bias, $strides, $padding, $explicit_paddings, $dilations)
>;

def FuseKerasConv2DOpPattern1 : Pat<
    (TF_AddV2Op
        (TF_Conv2DOp $x, $weight, $strides, $use_cudnn_on_gpu, $padding, $explicit_paddings, $data_format, $dilations),
        $bias
    ),

    (NCNN_KerasConv2DOp $x, $weight, $bias, $strides, $padding, $explicit_paddings, $dilations)
>;

def FuseKerasDenseOpPattern : Pat<
    (TF_BiasAddOp
        (TF_MatMulOp $x, $weight, $transpose_a, $transpose_b),
        $bias,
        $data_format_
    ),

    (NCNN_KerasDenseOp $x, $weight, $bias)
>;

def NonOneElementAttrPred : CPred<"$_self.cast<ElementsAttr>().getType().getNumElements() != 1">;

def NonOneElementAttr : ElementsAttrBase<And<[ElementsAttr.predicate, NonOneElementAttrPred]>, "Non Scalar ElementsAttr">;

def FuseKerasBatchNormOpPattern : Pat<
    (TF_AddV2Op
        (TF_MulOp
            $x,
            (TF_ConstOp:$gamma NonOneElementAttr)
        ),
        (TF_ConstOp:$bias NonOneElementAttr)
    ),

    (NCNN_KerasBatchNormOp $x, $gamma, $bias)
>;

def FuseInstanceNormPattern0 : Pat<
    (TF_MulOp
        (TF_RsqrtOp
            (TF_AddV2Op
                (TF_MeanOp
                    (TF_SquaredDifferenceOp
                        (TF_MeanOp:$mean
                            $x,
                            (TF_ConstOp:$reduce_axis ElementsAttr),
                            ConstBoolAttrTrue // keep_dims
                        ),
                        $x_
                    ),
                    $reduce_axis_,
                    ConstBoolAttrTrue // keep_dims
                ),
                (TF_ConstOp ElementsAttr:$epsilon)
            )
        ),
        (TF_SubOp $x__, $mean_)
    ),

    (NCNN_InstanceNormOp $x, (get_attr_f $epsilon)),

    [
        (EqualOperands $x, $x_),
        (EqualOperands $x, $x__),
        (EqualOperands $reduce_axis, $reduce_axis_),
        (EqualOperands $mean, $mean_)
    ]
>;

def FuseInstanceNormPattern1 : Pat<
    (TF_MulOp
        (TF_RsqrtOp
            (TF_AddV2Op
                (TF_MeanOp
                    (TF_SquaredDifferenceOp
                        $x_,
                        (TF_MeanOp:$mean
                            $x,
                            (TF_ConstOp:$reduce_axis ElementsAttr),
                            ConstBoolAttrTrue // keep_dims
                        )
                    ),
                    $reduce_axis_,
                    ConstBoolAttrTrue // keep_dims
                ),
                (TF_ConstOp ElementsAttr:$epsilon)
            )
        ),
        (TF_SubOp $x__, $mean_)
    ),

    (NCNN_InstanceNormOp $x, (get_attr_f $epsilon)),

    [
        (EqualOperands $x, $x_),
        (EqualOperands $x, $x__),
        (EqualOperands $reduce_axis, $reduce_axis_),
        (EqualOperands $mean, $mean_)
    ]
>;

def FuseInstanceNormAffinePattern : Pat<
    (TF_ReshapeOp
        (TF_AddV2Op
            (TF_MulOp
                $reshaped__,
                (TF_MulOp:$rsqrt_var_eps_gamma
                    (TF_RsqrtOp
                        (TF_AddV2Op
                            (TF_MeanOp
                                (TF_SquaredDifferenceOp
                                    $reshaped_,
                                    (TF_MeanOp:$mean
                                        (TF_ReshapeOp:$reshaped $x, (TF_ConstOp ElementsAttr)),
                                        (TF_ConstOp:$reduce_axis ElementsAttr),
                                        ConstBoolAttrTrue // keep_dims
                                    )
                                ),
                                $reduce_axis_,
                                ConstBoolAttrTrue // keep_dims
                            ),
                            (TF_ConstOp ElementsAttr:$epsilon)
                        )
                    ),
                    $gamma
                )
            ),
            (TF_SubOp
                $beta,
                (TF_MulOp $rsqrt_var_eps_gamma_, $mean_)
            )
        ),
        (TF_ConstOp ElementsAttr)
    ),

    (NCNN_InstanceNormAffineOp $x, $gamma, $beta, (get_attr_f $epsilon)),

    [
        (EqualOperands $reshaped, $reshaped_),
        (EqualOperands $reshaped, $reshaped__),
        (EqualOperands $reduce_axis, $reduce_axis_),
        (EqualOperands $rsqrt_var_eps_gamma, $rsqrt_var_eps_gamma_),
        (EqualOperands $mean, $mean_)
    ]
>;

def FuseSwishPattern : Pat<
    (TF_MulOp
        (TF_SigmoidOp $x),
        $x
    ),

    (NCNN_SwishOp $x)
>;

#endif // NCNN_REWRITER_TD
