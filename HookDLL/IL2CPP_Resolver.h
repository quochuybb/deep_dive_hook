#pragma once
#include <windows.h>
#include <stdint.h>

struct Il2CppClass;
struct Il2CppType;
struct FieldInfo;
struct Il2CppObject;
struct Il2CppDomain;
struct MethodInfo;
struct Il2CppException;

struct Il2CppImage {
    const char* name;
    const char* nameNoExt;
    void* assembly;
    int32_t typeStart;
    uint32_t typeCount;
    int32_t exportedTypeStart;
    uint32_t exportedTypeCount;
    int32_t customAttributeStart;
    uint32_t customAttributeCount;
    int32_t entryPointIndex;
    void* nameToClassHashTable;
    void* codeGenModule;
    uint32_t token;
    uint8_t dynamic;
};

struct Il2CppAssembly {
    Il2CppImage* image;
    uint32_t token;
    int32_t referencedAssemblyStart;
    int32_t referencedAssemblyCount;
    void* aname;
};

struct Il2CppString {
    void* object;
    int length;
    wchar_t chars[1];
};

struct Il2CppArray {
    void* obj_header_1;
    void* obj_header_2;
    void* bounds;
    size_t max_length;
    void* vector[1];
};

enum Il2CppTypeEnum {
    IL2CPP_TYPE_I4 = 0x8,
    IL2CPP_TYPE_R4 = 0xc,
    IL2CPP_TYPE_BOOLEAN = 0x2,
    IL2CPP_TYPE_STRING = 0xe,
    IL2CPP_TYPE_CLASS = 0x12,
    IL2CPP_TYPE_VALUETYPE = 0x11,
};

struct Il2CppType {
    void* data;
    unsigned int bits;
    Il2CppTypeEnum type;
};

typedef Il2CppDomain* (*t_il2cpp_domain_get)();
typedef Il2CppAssembly** (*t_il2cpp_domain_get_assemblies)(const Il2CppDomain* domain, size_t* size);
typedef Il2CppClass* (*t_il2cpp_class_from_name)(const Il2CppImage* image, const char* namespaze, const char* name);
typedef FieldInfo* (*t_il2cpp_class_get_fields)(Il2CppClass* klass, void** iter);
typedef const char* (*t_il2cpp_field_get_name)(FieldInfo* field);
typedef Il2CppType* (*t_il2cpp_field_get_type)(FieldInfo* field);
typedef void (*t_il2cpp_field_get_value)(Il2CppObject* obj, FieldInfo* field, void* value);
typedef void (*t_il2cpp_field_set_value)(Il2CppObject* obj, FieldInfo* field, void* value);
typedef void* (*t_il2cpp_resolve_icall)(const char* name);
typedef Il2CppClass* (*t_il2cpp_object_get_class)(Il2CppObject* obj);
typedef const char* (*t_il2cpp_class_get_name)(Il2CppClass* klass);
typedef void* (*t_il2cpp_type_get_object)(const Il2CppType* type);
typedef Il2CppType* (*t_il2cpp_class_get_type)(Il2CppClass* klass);
typedef void* (*t_il2cpp_thread_attach)(Il2CppDomain* domain);
typedef MethodInfo* (*t_il2cpp_class_get_method_from_name)(Il2CppClass* klass, const char* name, int argsCount);
typedef Il2CppObject* (*t_il2cpp_runtime_invoke)(const MethodInfo* method, void* obj, void** params, Il2CppException** exc);
typedef void* (*t_il2cpp_object_unbox)(Il2CppObject* obj);

extern t_il2cpp_domain_get il2cpp_domain_get;
extern t_il2cpp_domain_get_assemblies il2cpp_domain_get_assemblies;
extern t_il2cpp_class_from_name il2cpp_class_from_name;
extern t_il2cpp_class_get_fields il2cpp_class_get_fields;
extern t_il2cpp_field_get_name il2cpp_field_get_name;
extern t_il2cpp_field_get_type il2cpp_field_get_type;
extern t_il2cpp_field_get_value il2cpp_field_get_value;
extern t_il2cpp_field_set_value il2cpp_field_set_value;
extern t_il2cpp_resolve_icall il2cpp_resolve_icall;
extern t_il2cpp_object_get_class il2cpp_object_get_class;
extern t_il2cpp_class_get_name il2cpp_class_get_name;
extern t_il2cpp_type_get_object il2cpp_type_get_object;
extern t_il2cpp_class_get_type il2cpp_class_get_type;
extern t_il2cpp_thread_attach il2cpp_thread_attach;
extern t_il2cpp_class_get_method_from_name il2cpp_class_get_method_from_name;
extern t_il2cpp_runtime_invoke il2cpp_runtime_invoke;
extern t_il2cpp_object_unbox il2cpp_object_unbox;

void SetupIL2CPP();