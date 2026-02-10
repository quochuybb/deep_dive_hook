#pragma once
#include <windows.h>

struct Il2CppDomain;
struct Il2CppAssembly;
struct Il2CppImage;
struct Il2CppClass;
struct Il2CppMethod;
struct FieldInfo;
struct Il2CppObject;

typedef Il2CppDomain* (*t_il2cpp_domain_get)();
typedef Il2CppAssembly** (*t_il2cpp_domain_get_assemblies)(const Il2CppDomain* domain, size_t* size);
typedef Il2CppClass* (*t_il2cpp_class_from_name)(const Il2CppImage* image, const char* namespaze, const char* name);
typedef FieldInfo* (*t_il2cpp_class_get_fields)(Il2CppClass* klass, void** iter);
typedef void (*t_il2cpp_field_get_value)(Il2CppObject* obj, FieldInfo* field, void* value);
typedef void (*t_il2cpp_field_set_value)(Il2CppObject* obj, FieldInfo* field, void* value);
typedef char* (*t_il2cpp_field_get_name)(FieldInfo* field);
typedef void* (*t_il2cpp_resolve_icall)(const char* name);

extern t_il2cpp_domain_get il2cpp_domain_get;
extern t_il2cpp_class_from_name il2cpp_class_from_name;
extern t_il2cpp_class_get_fields il2cpp_class_get_fields;
extern t_il2cpp_field_get_value il2cpp_field_get_value;
extern t_il2cpp_field_set_value il2cpp_field_set_value;
extern t_il2cpp_field_get_name il2cpp_field_get_name;
extern t_il2cpp_resolve_icall il2cpp_resolve_icall;

void SetupIL2CPP() {
    HMODULE hGameAssembly = GetModuleHandleA("GameAssembly.dll");

    il2cpp_domain_get = (t_il2cpp_domain_get)GetProcAddress(hGameAssembly, "il2cpp_domain_get");
    il2cpp_class_from_name = (t_il2cpp_class_from_name)GetProcAddress(hGameAssembly, "il2cpp_class_from_name");
    il2cpp_class_get_fields = (t_il2cpp_class_get_fields)GetProcAddress(hGameAssembly, "il2cpp_class_get_fields");
    il2cpp_field_get_value = (t_il2cpp_field_get_value)GetProcAddress(hGameAssembly, "il2cpp_field_get_value");
    il2cpp_field_set_value = (t_il2cpp_field_set_value)GetProcAddress(hGameAssembly, "il2cpp_field_set_value");
    il2cpp_field_get_name = (t_il2cpp_field_get_name)GetProcAddress(hGameAssembly, "il2cpp_field_get_name");
    il2cpp_resolve_icall = (t_il2cpp_resolve_icall)GetProcAddress(hGameAssembly, "il2cpp_resolve_icall");
}