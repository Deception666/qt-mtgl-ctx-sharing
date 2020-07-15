#ifndef _COLOR_BUFFER_DATA_H_
#define _COLOR_BUFFER_DATA_H_

#include <cstdint>
#include <vector>

struct ColorBufferData
{
   uint32_t id_ { 0 };
   uint32_t width_ { 0 };
   uint32_t height_ { 0 };
   std::vector< uint32_t > data_;
};

#endif // _COLOR_BUFFER_DATA_H_
