#include "pch.h"
#include "IL2CPP_Resolver.h"

t_il2cpp_domain_get il2cpp_domain_get = nullptr;
t_il2cpp_domain_get_assemblies il2cpp_domain_get_assemblies = nullptr;
t_il2cpp_class_from_name il2cpp_class_from_name = nullptr;
t_il2cpp_class_get_fields il2cpp_class_get_fields = nullptr;
t_il2cpp_field_get_name il2cpp_field_get_name = nullptr;
t_il2cpp_field_get_type il2cpp_field_get_type = nullptr;
t_il2cpp_field_get_value il2cpp_field_get_value = nullptr;
t_il2cpp_field_set_value il2cpp_field_set_value = nullptr;
t_il2cpp_resolve_icall il2cpp_resolve_icall = nullptr;
t_il2cpp_object_get_class il2cpp_object_get_class = nullptr;
t_il2cpp_class_get_name il2cpp_class_get_name = nullptr;
t_il2cpp_type_get_object il2cpp_type_get_object = nullptr;
t_il2cpp_class_get_type il2cpp_class_get_type = nullptr;
t_il2cpp_thread_attach il2cpp_thread_attach = nullptr;
t_il2cpp_class_get_method_from_name il2cpp_class_get_method_from_name = nullptr;
t_il2cpp_runtime_invoke il2cpp_runtime_invoke = nullptr;
t_il2cpp_object_unbox il2cpp_object_unbox = nullptr;

void SetupIL2CPP() {
    HMODULE hGameAssembly = GetModuleHandleA("GameAssembly.dll");
    if (!hGameAssembly) return;

    il2cpp_domain_get = (t_il2cpp_domain_get)GetProcAddress(hGameAssembly, "il2cpp_domain_get");
    il2cpp_domain_get_assemblies = (t_il2cpp_domain_get_assemblies)GetProcAddress(hGameAssembly, "il2cpp_domain_get_assemblies");
    il2cpp_thread_attach = (t_il2cpp_thread_attach)GetProcAddress(hGameAssembly, "il2cpp_thread_attach");
    il2cpp_class_from_name = (t_il2cpp_class_from_name)GetProcAddress(hGameAssembly, "il2cpp_class_from_name");
    il2cpp_class_get_fields = (t_il2cpp_class_get_fields)GetProcAddress(hGameAssembly, "il2cpp_class_get_fields");
    il2cpp_field_get_name = (t_il2cpp_field_get_name)GetProcAddress(hGameAssembly, "il2cpp_field_get_name");
    il2cpp_field_get_type = (t_il2cpp_field_get_type)GetProcAddress(hGameAssembly, "il2cpp_field_get_type");
    il2cpp_field_get_value = (t_il2cpp_field_get_value)GetProcAddress(hGameAssembly, "il2cpp_field_get_value");
    il2cpp_field_set_value = (t_il2cpp_field_set_value)GetProcAddress(hGameAssembly, "il2cpp_field_set_value");
    il2cpp_resolve_icall = (t_il2cpp_resolve_icall)GetProcAddress(hGameAssembly, "il2cpp_resolve_icall");
    il2cpp_object_get_class = (t_il2cpp_object_get_class)GetProcAddress(hGameAssembly, "il2cpp_object_get_class");
    il2cpp_class_get_name = (t_il2cpp_class_get_name)GetProcAddress(hGameAssembly, "il2cpp_class_get_name");
    il2cpp_type_get_object = (t_il2cpp_type_get_object)GetProcAddress(hGameAssembly, "il2cpp_type_get_object");
    il2cpp_class_get_type = (t_il2cpp_class_get_type)GetProcAddress(hGameAssembly, "il2cpp_class_get_type");
    il2cpp_class_get_method_from_name = (t_il2cpp_class_get_method_from_name)GetProcAddress(hGameAssembly, "il2cpp_class_get_method_from_name");
    il2cpp_runtime_invoke = (t_il2cpp_runtime_invoke)GetProcAddress(hGameAssembly, "il2cpp_runtime_invoke");
    il2cpp_object_unbox = (t_il2cpp_object_unbox)GetProcAddress(hGameAssembly, "il2cpp_object_unbox");
}