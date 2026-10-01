#include "npgpu_runtime.hpp"
#include "npruntime.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// Minimal CUDA Driver API types & signatures
typedef int CUresult;
typedef int CUdevice;
typedef void* CUcontext;
typedef void* CUmodule;
typedef void* CUfunction;
typedef unsigned long long CUdeviceptr;

#define CUDA_SUCCESS 0

typedef CUresult (*cuInit_t)(unsigned int);
typedef CUresult (*cuDeviceGetCount_t)(int*);
typedef CUresult (*cuDeviceGet_t)(CUdevice*, int);
typedef CUresult (*cuDeviceGetName_t)(char*, int, CUdevice);
typedef CUresult (*cuCtxCreate_v2_t)(CUcontext*, unsigned int, CUdevice);
typedef CUresult (*cuCtxSynchronize_t)();
typedef CUresult (*cuModuleLoadData_t)(CUmodule*, const void*);
typedef CUresult (*cuModuleUnload_t)(CUmodule);
typedef CUresult (*cuModuleGetFunction_t)(CUfunction*, CUmodule, const char*);
typedef CUresult (*cuMemAlloc_v2_t)(CUdeviceptr*, size_t);
typedef CUresult (*cuMemFree_v2_t)(CUdeviceptr);
typedef CUresult (*cuMemcpyHtoD_v2_t)(CUdeviceptr, const void*, size_t);
typedef CUresult (*cuMemcpyDtoH_v2_t)(void*, CUdeviceptr, size_t);
typedef CUresult (*cuLaunchKernel_t)(CUfunction, unsigned int, unsigned int, unsigned int,
                                    unsigned int, unsigned int, unsigned int,
                                    unsigned int, void*, void**, void**);

struct CudaDriver {
    bool loaded = false;
    void* libHandle = nullptr;
    CUcontext context = nullptr;
    std::mutex mtx;

    cuInit_t cuInit = nullptr;
    cuDeviceGetCount_t cuDeviceGetCount = nullptr;
    cuDeviceGet_t cuDeviceGet = nullptr;
    cuDeviceGetName_t cuDeviceGetName = nullptr;
    cuCtxCreate_v2_t cuCtxCreate_v2 = nullptr;
    cuCtxSynchronize_t cuCtxSynchronize = nullptr;
    cuModuleLoadData_t cuModuleLoadData = nullptr;
    cuModuleUnload_t cuModuleUnload = nullptr;
    cuModuleGetFunction_t cuModuleGetFunction = nullptr;
    cuMemAlloc_v2_t cuMemAlloc_v2 = nullptr;
    cuMemFree_v2_t cuMemFree_v2 = nullptr;
    cuMemcpyHtoD_v2_t cuMemcpyHtoD_v2 = nullptr;
    cuMemcpyDtoH_v2_t cuMemcpyDtoH_v2 = nullptr;
    cuLaunchKernel_t cuLaunchKernel = nullptr;

    bool init() {
        std::lock_guard<std::mutex> lock(mtx);
        if (loaded) return true;

#ifdef _WIN32
        libHandle = (void*)LoadLibraryA("nvcuda.dll");
        if (!libHandle) return false;
        #define GET_SYM(sym) sym = (sym##_t)GetProcAddress((HMODULE)libHandle, #sym)
#else
        libHandle = dlopen("libcuda.so.1", RTLD_NOW);
        if (!libHandle) libHandle = dlopen("/usr/lib/wsl/lib/libcuda.so.1", RTLD_NOW);
        if (!libHandle) return false;
        #define GET_SYM(sym) sym = (sym##_t)dlsym(libHandle, #sym)
#endif

        GET_SYM(cuInit);
        GET_SYM(cuDeviceGetCount);
        GET_SYM(cuDeviceGet);
        GET_SYM(cuDeviceGetName);
        GET_SYM(cuCtxCreate_v2);
        GET_SYM(cuCtxSynchronize);
        GET_SYM(cuModuleLoadData);
        GET_SYM(cuModuleUnload);
        GET_SYM(cuModuleGetFunction);
        GET_SYM(cuMemAlloc_v2);
        GET_SYM(cuMemFree_v2);
        GET_SYM(cuMemcpyHtoD_v2);
        GET_SYM(cuMemcpyDtoH_v2);
        GET_SYM(cuLaunchKernel);

        if (!cuInit || cuInit(0) != CUDA_SUCCESS) {
            return false;
        }

        int count = 0;
        if (!cuDeviceGetCount || cuDeviceGetCount(&count) != CUDA_SUCCESS || count == 0) {
            return false;
        }

        CUdevice dev;
        if (!cuDeviceGet || cuDeviceGet(&dev, 0) != CUDA_SUCCESS) {
            return false;
        }

        if (!cuCtxCreate_v2 || cuCtxCreate_v2(&context, 0, dev) != CUDA_SUCCESS) {
            return false;
        }

        loaded = true;
        return true;
    }
};

static CudaDriver gCuda;

extern "C" {

int64_t np_rt_gpu_device_count() {
    if (!gCuda.init()) return 0;
    int count = 0;
    if (gCuda.cuDeviceGetCount && gCuda.cuDeviceGetCount(&count) == CUDA_SUCCESS) {
        return count;
    }
    return 0;
}

void* np_rt_gpu_device_name(int64_t dev_id) {
    if (!gCuda.init()) return new np_string("Unknown GPU");
    CUdevice dev;
    if (gCuda.cuDeviceGet && gCuda.cuDeviceGet(&dev, (int)dev_id) == CUDA_SUCCESS) {
        char name[256] = {0};
        if (gCuda.cuDeviceGetName && gCuda.cuDeviceGetName(name, sizeof(name), dev) == CUDA_SUCCESS) {
            return new np_string(name);
        }
    }
    return new np_string("NVIDIA GPU");
}

bool np_rt_gpu_is_available() {
    return gCuda.init();
}

struct ArrayBufferBinding {
    np_var* targetArray;
    bool isDouble;
    bool isInt;
    std::vector<float> f_data;
    std::vector<double> d_data;
    std::vector<int64_t> i_data;
    CUdeviceptr d_ptr;
    size_t bytes;
};

void np_rt_gpu_launch(void* ptx_str_ptr, void* kernel_name_str_ptr, int64_t gridDimX, int64_t blockDimX, void* argList) {
    if (!gCuda.init()) {
        std::cerr << "GPU Runtime Error: No compatible NVIDIA GPU driver found (libcuda.so.1).\n";
        return;
    }

    std::string ptxStr = "";
    if (ptx_str_ptr) {
        ptxStr = static_cast<np_string*>(ptx_str_ptr)->c_str();
    }
    std::string kernelName = "";
    if (kernel_name_str_ptr) {
        kernelName = static_cast<np_string*>(kernel_name_str_ptr)->c_str();
    }

    if (ptxStr.empty() || kernelName.empty()) {
        std::cerr << "GPU Error: Invalid PTX code or kernel name provided for launch.\n";
        return;
    }

    CUmodule cuMod;
    if (gCuda.cuModuleLoadData(&cuMod, ptxStr.c_str()) != CUDA_SUCCESS) {
        std::cerr << "GPU Error: Failed to compile/load PTX module into CUDA driver.\n";
        return;
    }

    CUfunction cuFunc;
    if (gCuda.cuModuleGetFunction(&cuFunc, cuMod, kernelName.c_str()) != CUDA_SUCCESS) {
        std::cerr << "GPU Error: Kernel function '" << kernelName << "' not found in GPU module.\n";
        gCuda.cuModuleUnload(cuMod);
        return;
    }

    std::vector<ArrayBufferBinding> arrayBindings;
    std::vector<CUdeviceptr> devicePointers;
    std::vector<int64_t> scalarInts;
    std::vector<double> scalarDoubles;
    std::vector<void*> kernelParams;

    struct ParamRef {
        int kind; // 0 = ptr, 1 = int, 2 = double
        size_t index;
    };
    std::vector<ParamRef> paramRefs;

    if (argList) {
        auto* argsVar = static_cast<np_var*>(argList);
        if (std::holds_alternative<ListPtr>(argsVar->v) && std::get<ListPtr>(argsVar->v)) {
            const auto& listVec = std::get<ListPtr>(argsVar->v)->vec;
            for (size_t i = 0; i < listVec.size(); ++i) {
                const auto& item = listVec[i];
                if (std::holds_alternative<ListPtr>(item.v) && std::get<ListPtr>(item.v)) {
                    // It's an array parameter!
                    auto const& arr = std::get<ListPtr>(item.v)->vec;
                    ArrayBufferBinding binding;
                    binding.targetArray = const_cast<np_var*>(&item);
                    binding.isDouble = false;
                    binding.isInt = false;

                    // Detect element type
                    if (!arr.empty()) {
                        if (std::holds_alternative<double>(arr[0].v)) binding.isDouble = true;
                        else if (std::holds_alternative<int64_t>(arr[0].v)) binding.isInt = true;
                    }

                    if (binding.isDouble) {
                        binding.d_data.reserve(arr.size());
                        for (const auto& el : arr) binding.d_data.push_back(static_cast<double>(el));
                        binding.bytes = binding.d_data.size() * sizeof(double);
                    } else if (binding.isInt) {
                        binding.i_data.reserve(arr.size());
                        for (const auto& el : arr) binding.i_data.push_back(static_cast<int64_t>(el));
                        binding.bytes = binding.i_data.size() * sizeof(int64_t);
                    } else {
                        // Float32
                        binding.f_data.reserve(arr.size());
                        for (const auto& el : arr) binding.f_data.push_back((float)static_cast<double>(el));
                        binding.bytes = binding.f_data.size() * sizeof(float);
                    }

                    if (binding.bytes == 0) binding.bytes = sizeof(float); // minimum alloc

                    gCuda.cuMemAlloc_v2(&binding.d_ptr, binding.bytes);
                    if (binding.isDouble) {
                        gCuda.cuMemcpyHtoD_v2(binding.d_ptr, binding.d_data.data(), binding.bytes);
                    } else if (binding.isInt) {
                        gCuda.cuMemcpyHtoD_v2(binding.d_ptr, binding.i_data.data(), binding.bytes);
                    } else {
                        gCuda.cuMemcpyHtoD_v2(binding.d_ptr, binding.f_data.data(), binding.bytes);
                    }

                    devicePointers.push_back(binding.d_ptr);
                    arrayBindings.push_back(binding);
                    paramRefs.push_back({0, devicePointers.size() - 1});
                } else if (std::holds_alternative<int64_t>(item.v)) {
                    scalarInts.push_back(std::get<int64_t>(item.v));
                    paramRefs.push_back({1, scalarInts.size() - 1});
                } else if (std::holds_alternative<double>(item.v)) {
                    scalarDoubles.push_back(std::get<double>(item.v));
                    paramRefs.push_back({2, scalarDoubles.size() - 1});
                } else {
                    scalarInts.push_back(0);
                    paramRefs.push_back({1, scalarInts.size() - 1});
                }
            }
        }
    }

    for (const auto& ref : paramRefs) {
        if (ref.kind == 0) {
            kernelParams.push_back(&devicePointers[ref.index]);
        } else if (ref.kind == 1) {
            kernelParams.push_back(&scalarInts[ref.index]);
        } else if (ref.kind == 2) {
            kernelParams.push_back(&scalarDoubles[ref.index]);
        }
    }

    unsigned int gX = (unsigned int)(gridDimX > 0 ? gridDimX : 1);
    unsigned int bX = (unsigned int)(blockDimX > 0 ? blockDimX : 256);

    CUresult launchRes = gCuda.cuLaunchKernel(cuFunc, gX, 1, 1, bX, 1, 1, 0, nullptr, kernelParams.data(), nullptr);
    if (launchRes != CUDA_SUCCESS) {
        std::cerr << "GPU Error: cuLaunchKernel failed with code " << launchRes << "\n";
    }

    gCuda.cuCtxSynchronize();

    // Copy back device buffers to caller array
    for (size_t b = 0; b < arrayBindings.size(); ++b) {
        auto& binding = arrayBindings[b];
        if (binding.isDouble) {
            gCuda.cuMemcpyDtoH_v2(binding.d_data.data(), binding.d_ptr, binding.bytes);
            auto* listObj = std::get<ListPtr>(binding.targetArray->v).get();
            for (size_t i = 0; i < binding.d_data.size(); ++i) {
                if (i < listObj->vec.size()) listObj->vec[i] = binding.d_data[i];
            }
        } else if (binding.isInt) {
            gCuda.cuMemcpyDtoH_v2(binding.i_data.data(), binding.d_ptr, binding.bytes);
            auto* listObj = std::get<ListPtr>(binding.targetArray->v).get();
            for (size_t i = 0; i < binding.i_data.size(); ++i) {
                if (i < listObj->vec.size()) listObj->vec[i] = binding.i_data[i];
            }
        } else {
            gCuda.cuMemcpyDtoH_v2(binding.f_data.data(), binding.d_ptr, binding.bytes);
            auto* listObj = std::get<ListPtr>(binding.targetArray->v).get();
            for (size_t i = 0; i < binding.f_data.size(); ++i) {
                if (i < listObj->vec.size()) listObj->vec[i] = (double)binding.f_data[i];
            }
        }
        gCuda.cuMemFree_v2(binding.d_ptr);
    }

    gCuda.cuModuleUnload(cuMod);
}

}
