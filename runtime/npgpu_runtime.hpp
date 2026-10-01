#pragma once
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

int64_t np_rt_gpu_device_count();
void* np_rt_gpu_device_name(int64_t dev_id);
bool np_rt_gpu_is_available();
void np_rt_gpu_launch(void* ptx_str_ptr, void* kernel_name_str_ptr, int64_t gridDimX, int64_t blockDimX, void* argList);

#ifdef __cplusplus
}
#endif
