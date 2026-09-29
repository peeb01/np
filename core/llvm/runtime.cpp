#include "llvm_codegen.hpp"

void LLVMCodeGen::declareRuntime() {
    auto i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(Context), 0);
    auto i8PtrPtrTy = llvm::PointerType::get(i8PtrTy, 0);
    auto i64Ty = llvm::Type::getInt64Ty(Context);
    auto doubleTy = llvm::Type::getDoubleTy(Context);
    auto i1Ty = llvm::Type::getInt1Ty(Context);
    auto voidTy = llvm::Type::getVoidTy(Context);
    
    // Helper to declare extern "C" functions
    auto declareFunc = [&](const std::string& name, llvm::Type* retTy, const std::vector<llvm::Type*>& argTys) {
        auto ft = llvm::FunctionType::get(retTy, argTys, false);
        auto f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, TheModule);
        RuntimeFunctions[name] = f;
    };
    
    // String API
    declareFunc("np_rt_string_create", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_string_destroy", voidTy, {i8PtrTy});
    declareFunc("np_rt_string_concat", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_string_concat_char", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_string_concat_char_lhs", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_string_c_str", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_string_len", i64Ty, {i8PtrTy});
    declareFunc("np_rt_string_slice", i8PtrTy, {i8PtrTy, i64Ty, i64Ty});
    declareFunc("np_rt_string_eq", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_string_lt", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_string_contains", i1Ty, {i8PtrTy, i8PtrTy});
    
    // np_var API
    declareFunc("np_rt_var_create_int", i8PtrTy, {i64Ty});
    declareFunc("np_rt_var_create_float", i8PtrTy, {doubleTy});
    declareFunc("np_rt_var_create_string", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_var_create_bool", i8PtrTy, {i1Ty});
    declareFunc("np_rt_var_create_list", i8PtrTy, {});
    declareFunc("np_rt_var_create_list_size", i8PtrTy, {i64Ty});
    declareFunc("np_rt_var_create_dict", i8PtrTy, {});
    declareFunc("np_rt_var_create_int128", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_var_create_int256", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_var_destroy", voidTy, {i8PtrTy});
    
    declareFunc("np_rt_var_append", voidTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_pop", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_var_clear", voidTy, {i8PtrTy});
    declareFunc("np_rt_var_keys", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_var_values", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_var_sort", voidTy, {i8PtrTy});
    declareFunc("np_rt_var_reverse", voidTy, {i8PtrTy});
    declareFunc("np_rt_var_contains", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_split", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_join", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_trim", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_var_get_index", i8PtrTy, {i8PtrTy, i64Ty});
    declareFunc("np_rt_var_set_index", voidTy, {i8PtrTy, i64Ty, i8PtrTy});
    declareFunc("np_rt_var_get_key", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_set_key", voidTy, {i8PtrTy, i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_len", i64Ty, {i8PtrTy});
    declareFunc("np_rt_var_slice", i8PtrTy, {i8PtrTy, i64Ty, i64Ty});
    declareFunc("np_rt_var_shape", i8PtrTy, {i8PtrTy});
    
    // np_var Operators
    declareFunc("np_rt_var_add", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_sub", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_mul", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_div", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_mod", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_pow", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_gt", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_lt", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_ge", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_le", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_eq", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_ne", i1Ty, {i8PtrTy, i8PtrTy});
    
    // Printing API
    declareFunc("np_rt_print_int", voidTy, {i64Ty});
    declareFunc("np_rt_print_float", voidTy, {doubleTy});
    declareFunc("np_rt_print_bool", voidTy, {i1Ty});
    declareFunc("np_rt_print_string", voidTy, {i8PtrTy});
    declareFunc("np_rt_print_var", voidTy, {i8PtrTy});
    declareFunc("np_rt_assert_fail", voidTy, {i64Ty, i8PtrTy});
    
    // Conversion API
    declareFunc("np_rt_to_int_string", i64Ty, {i8PtrTy});
    declareFunc("np_rt_to_int_var", i64Ty, {i8PtrTy});
    declareFunc("np_rt_to_float_string", doubleTy, {i8PtrTy});
    declareFunc("np_rt_to_float_var", doubleTy, {i8PtrTy});
    declareFunc("np_rt_to_string_int", i8PtrTy, {i64Ty});
    declareFunc("np_rt_to_string_float", i8PtrTy, {doubleTy});
    declareFunc("np_rt_to_string_var", i8PtrTy, {i8PtrTy});
    
    // Input API
    declareFunc("np_rt_input_int", i64Ty, {});
    declareFunc("np_rt_input_float", doubleTy, {});
    declareFunc("np_rt_input_string", i8PtrTy, {});
    
    // Math functions
    declareFunc("sqrt", doubleTy, {doubleTy});
    declareFunc("fabs", doubleTy, {doubleTy});
    declareFunc("round", doubleTy, {doubleTy});
    declareFunc("pow", doubleTy, {doubleTy, doubleTy});
    declareFunc("np_rt_min", doubleTy, {doubleTy, doubleTy});
    declareFunc("np_rt_max", doubleTy, {doubleTy, doubleTy});
    
    // Modules
    declareFunc("np_rt_sys_init_args", voidTy, {llvm::Type::getInt32Ty(Context), i8PtrPtrTy});
    declareFunc("np_rt_sys_get_argv", i8PtrPtrTy, {});
    declareFunc("np_rt_time_now", doubleTy, {});
    declareFunc("np_rt_time_sleep", voidTy, {doubleTy});
    declareFunc("np_rt_time_format", i8PtrTy, {doubleTy, i8PtrTy});
    declareFunc("np_rt_json_stringify", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_json_parse", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_regex_match", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_regex_find", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_regex_replace", i8PtrTy, {i8PtrTy, i8PtrTy, i8PtrTy});
    declareFunc("np_rt_type_var", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_read_file", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_write_file", i64Ty, {i8PtrTy, i8PtrTy});

    // Networking Socket API
    declareFunc("np_rt_net_listen", i64Ty, {i64Ty});
    declareFunc("np_rt_net_accept", i64Ty, {i64Ty});
    declareFunc("np_rt_net_connect", i64Ty, {i8PtrTy, i64Ty});
    declareFunc("np_rt_net_send", i64Ty, {i64Ty, i8PtrTy});
    declareFunc("np_rt_net_recv", i8PtrTy, {i64Ty, i64Ty});
    declareFunc("np_rt_net_close", voidTy, {i64Ty});

    // Concurrency Channels & Goroutines API
    declareFunc("np_rt_chan_create", i8PtrTy, {i64Ty});
    declareFunc("np_rt_chan_send", voidTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_chan_recv", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_chan_close", voidTy, {i8PtrTy});
    declareFunc("np_rt_go_spawn", voidTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_create_ptr", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_to_ptr_var", i8PtrTy, {i8PtrTy});

    // OS & Crypto API
    declareFunc("np_rt_os_exec", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_os_system", i64Ty, {i8PtrTy});
    declareFunc("np_rt_os_getenv", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_crypto_sha256", i8PtrTy, {i8PtrTy});
}
