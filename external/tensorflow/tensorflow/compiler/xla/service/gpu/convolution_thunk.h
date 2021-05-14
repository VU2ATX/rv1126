/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_COMPILER_XLA_SERVICE_GPU_CONVOLUTION_THUNK_H_
#define TENSORFLOW_COMPILER_XLA_SERVICE_GPU_CONVOLUTION_THUNK_H_

#include "tensorflow/compiler/xla/service/buffer_assignment.h"
#include "tensorflow/compiler/xla/service/gpu/buffer_allocations.h"
#include "tensorflow/compiler/xla/service/gpu/cudnn_convolution_runner.h"
#include "tensorflow/compiler/xla/service/gpu/gpu_executable.h"
#include "tensorflow/compiler/xla/service/gpu/thunk.h"
#include "tensorflow/compiler/xla/service/hlo_instruction.h"
#include "tensorflow/compiler/xla/types.h"
#include "tensorflow/compiler/xla/xla_data.pb.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/gtl/optional.h"
#include "tensorflow/core/platform/stream_executor_no_cuda.h"

namespace xla {
namespace gpu {

// This class stores everything that StreamExecutor needs to launch a BNN
// convolution. It is generated by IrEmitter.
//
// This is thread-compatible.
class ConvolutionThunk : public Thunk {
 public:
  // Constructs a thunk for launching a DNN convolution.  When run, it will
  // write a tuple (result, scratch_memory) into `tuple_result_buffer`.
  //
  // `algorithm` is a cudnn algorithm number.  `algorithm == -1` indicates that
  // we should use the default (i.e. baseline) cudnn algorithm.
  //
  // Note that "output" here doesn't refer to the output from running this
  // thunk, but rather to the "output" of a hypothetical forward convolution
  // that corresponds to this input+filter+output triple.  That is, the result
  // generated by this thunk is "output" for forward convs, "input" for
  // backward-input convs, and "filter" for backward-filter convs.
  //
  // Semantics of null hlo_instruction argument are as in Thunk.
  ConvolutionThunk(CudnnConvKind convolution_kind,
                   const BufferAllocation::Slice& input_buffer,
                   const BufferAllocation::Slice& filter_buffer,
                   const BufferAllocation::Slice& output_buffer,
                   const BufferAllocation::Slice& tuple_result_buffer,
                   const BufferAllocation::Slice& scratch_buffer,
                   const Shape& input_shape, const Shape& filter_shape,
                   const Shape& output_shape, const Window& window,
                   const ConvolutionDimensionNumbers& dim_nums, int64 algorithm,
                   bool tensor_ops_enabled, const HloInstruction* hlo);

  ConvolutionThunk(const ConvolutionThunk&) = delete;
  ConvolutionThunk& operator=(const ConvolutionThunk&) = delete;

  // Does the convolution for the thunk on "stream".
  Status ExecuteOnStream(const BufferAllocations& buffer_allocations,
                         se::Stream* stream) override;

 private:
  class ScratchAllocator;

  Status Convolve(const se::dnn::BatchDescriptor& input_descriptor,
                  se::DeviceMemory<float> input_data,
                  const se::dnn::FilterDescriptor& filter_descriptor,
                  se::DeviceMemory<float> filter_data,
                  const se::dnn::BatchDescriptor& output_descriptor,
                  se::DeviceMemory<float> output_data,
                  const se::dnn::ConvolutionDescriptor& convolution_descriptor,
                  const se::dnn::AlgorithmConfig& algorithm_config,
                  se::Stream* stream, ScratchAllocator* scratch_allocator,
                  se::dnn::ProfileResult* profile_result);

  const CudnnConvKind convolution_kind_;

  const BufferAllocation::Slice input_buffer_;
  const BufferAllocation::Slice filter_buffer_;
  const BufferAllocation::Slice output_buffer_;
  const BufferAllocation::Slice tuple_result_buffer_;
  const BufferAllocation::Slice scratch_buffer_;

  const Shape input_shape_;
  const Shape filter_shape_;
  const Shape output_shape_;

  const Window window_;
  const ConvolutionDimensionNumbers dim_nums_;
  int64 algorithm_;
  bool tensor_ops_enabled_;
};

}  // namespace gpu
}  // namespace xla

#endif  // TENSORFLOW_COMPILER_XLA_SERVICE_GPU_CONVOLUTION_THUNK_H_