/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2011, Willow Garage, Inc.
 *
 *  All rights reserved. 
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <pcl/recognition/quantizable_modality.h>
#include <cstddef>
#include <string.h>

#if __AVX2__
#include <immintrin.h>
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
pcl::QuantizableModality::QuantizableModality ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////
pcl::QuantizableModality::~QuantizableModality ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////
pcl::QuantizedMap::QuantizedMap ()
  : data_ (0), width_ (0), height_ (0)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////
pcl::QuantizedMap::QuantizedMap (const QuantizedMap & copy_me)
  : data_ (0), width_ (copy_me.width_), height_ (copy_me.height_)
{
  data_.insert (data_.begin (), copy_me.data_.begin (), copy_me.data_.end ());
}

//////////////////////////////////////////////////////////////////////////////////////////////
pcl::QuantizedMap::QuantizedMap (const size_t width, const size_t height)
  : data_ (width*height), width_ (width), height_ (height)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////
pcl::QuantizedMap::~QuantizedMap ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////
void
pcl::QuantizedMap::
resize (const size_t width, const size_t height)
{
  data_.resize (width*height);
  width_ = width;
  height_ = height;
}

//////////////////////////////////////////////////////////////////////////////////////////////
void
pcl::QuantizedMap::
spreadQuantizedMap (const QuantizedMap & input_map, QuantizedMap & output_map, const size_t spreading_size)
{
  // TODO: implement differently (as in opencv)
  const size_t width = input_map.getWidth ();
  const size_t height = input_map.getHeight ();
  const size_t half_spreading_size = spreading_size / 2;

  QuantizedMap tmp_map (width, height);
  output_map.resize (width, height);

  const int col_index_max = width-spreading_size-1;
  const int col_index_max_avx2 = width-spreading_size-1-32;
  const int row_index_max = height-spreading_size-1;

  //TODO: We should have avx2 + sse, so that very few loops are done without optimization.
  //      Right now, too much time is spend on non-avx2 loop? check!
  for (int row_index = 0; row_index < row_index_max; ++row_index)
  {
    int col_index = 0;
#if __AVX2__
    for (; col_index <= col_index_max_avx2; col_index+=32) {
      __m256i __value = _mm256_setzero_si256();
      const unsigned char * data_ptr = &(input_map (col_index, row_index));
      for (size_t spreading_index = 0; spreading_index < spreading_size; ++spreading_index, ++data_ptr)
      {
        __value = _mm256_or_si256(__value, _mm256_loadu_si256((__m256i*) data_ptr));
      }
      _mm256_storeu_si256((__m256i*) &tmp_map(col_index + half_spreading_size, row_index), __value);
    }
#endif
    for (; col_index < col_index_max; ++col_index)
    {
      unsigned char value = 0;
      const unsigned char * data_ptr = &(input_map (col_index, row_index));
      for (size_t spreading_index = 0; spreading_index < spreading_size; ++spreading_index, ++data_ptr)
      {
        value |= *data_ptr;
      }

      tmp_map (col_index + half_spreading_size, row_index) = value;
    }
  }

  for (int row_index = 0; row_index < row_index_max; ++row_index)
  {
    int col_index = 0;
#if __AVX2__
    for (; col_index <= col_index_max_avx2; col_index+=32)
    {
      __m256i __value = _mm256_setzero_si256();
      const unsigned char * data_ptr = &(tmp_map (col_index, row_index));
      for (size_t spreading_index = 0; spreading_index < spreading_size; ++spreading_index, data_ptr += width)
      {
        __value = _mm256_or_si256(__value, _mm256_loadu_si256((__m256i*) data_ptr));
      }
      _mm256_storeu_si256((__m256i*) &output_map (col_index, row_index + half_spreading_size), __value);
    }
#endif

    for (; col_index < col_index_max; ++col_index)
    {
      unsigned char value = 0;
      const unsigned char * data_ptr = &(tmp_map (col_index, row_index));
      for (size_t spreading_index = 0; spreading_index < spreading_size; ++spreading_index, data_ptr += width)
      {
        value |= *data_ptr;
      }

      output_map (col_index, row_index + half_spreading_size) = value;
    }
  }
}
