#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Максимални размери (за MCU с повече памет, напр. STM32F4)
#define MAX_STACK_SIZE 256
#define MAX_LOCALS 64
#define MAX_GLOBALS 32
#define MAX_OBJECTS 32
#define MAX_ATTRS_PER_OBJECT 8
#define MAX_NAME_LENGTH 32

// Структура на BIN файла
typedef struct {
    uint32_t object_count;
    uint32_t bytecode_size;
    uint32_t constants_size;
    uint32_t names_size;
    uint32_t varnames_size;
} BinHeader;

typedef struct {
    uint32_t type;  // 1=bytecode, 2=int, 3=string, 4=float, 5=name, 6=varname
    uint32_t offset;
} ObjectEntry;

typedef struct {
    BinHeader header;
    ObjectEntry* objects;
    uint8_t* bytecode;
    uint8_t* constants;
    uint8_t* names;
    uint8_t* varnames;
} BinFile;

// Структура за низ
typedef struct {
    uint16_t length;
    const uint8_t* data;
} StringRef;

// Типове стойности
typedef union {
    int32_t i;          // Цяло число
    float f;            // Float
    uint32_t obj_idx;   // Индекс на обект
} Value;

// Атрибут на обект
typedef struct {
    uint8_t name[MAX_NAME_LENGTH];
    Value value;
} Attribute;

// Обект (клас или инстанция)
typedef struct {
    Attribute attrs[MAX_ATTRS_PER_OBJECT];
    uint8_t attr_count;
    uint32_t class_idx;  // Индекс на класа (0 за класове, иначе индекс на базов клас)
} Object;

// Глобална променлива
typedef struct {
    uint8_t name[MAX_NAME_LENGTH];
    Value value;
} GlobalVar;

// Виртуална машина
typedef struct {
    Value stack[MAX_STACK_SIZE];
    uint32_t stack_top;
    Value locals[MAX_LOCALS];
    GlobalVar globals[MAX_GLOBALS];
    uint32_t global_count;
    Object objects[MAX_OBJECTS];
    uint32_t object_count;
    BinFile* bin;  // Указател към BIN файла
} VM;

// Инструкции (от Python 3.x)
#define LOAD_CONST      1
#define STORE_FAST      100
#define LOAD_FAST       101
#define BINARY_ADD      23
#define RETURN_VALUE    83
#define LOAD_BUILD_CLASS 71
#define STORE_NAME      90
#define LOAD_NAME       101
#define STORE_ATTR      95
#define LOAD_ATTR       106
#define CALL_FUNCTION   131
#define MAKE_FUNCTION   132

// Инициализация на VM
void vm_init(VM* vm, BinFile* bin) {
    vm->stack_top = 0;
    vm->global_count = 0;
    vm->object_count = 0;
    vm->bin = bin;
    memset(vm->locals, 0, sizeof(vm->locals));
    memset(vm->globals, 0, sizeof(vm->globals));
    memset(vm->objects, 0, sizeof(vm->objects));
}

// Създаване на обект
uint32_t vm_create_object(VM* vm, uint32_t class_idx) {
    if (vm->object_count >= MAX_OBJECTS) return 0;
    uint32_t idx = vm->object_count++;
    vm->objects[idx].class_idx = class_idx;
    return idx;
}

// Управление на атрибути
void vm_set_attr(VM* vm, uint32_t obj_idx, const uint8_t* name, uint16_t name_len, Value value) {
    Object* obj = &vm->objects[obj_idx];
    for (uint8_t i = 0; i < obj->attr_count; i++) {
        if (strncmp((char*)obj->attrs[i].name, (char*)name, name_len) == 0) {
            obj->attrs[i].value = value;
            return;
        }
    }
    if (obj->attr_count < MAX_ATTRS_PER_OBJECT) {
        strncpy((char*)obj->attrs[obj->attr_count].name, (char*)name, name_len < MAX_NAME_LENGTH ? name_len : MAX_NAME_LENGTH - 1);
        obj->attrs[obj->attr_count].value = value;
        obj->attr_count++;
    }
}

Value vm_get_attr(VM* vm, uint32_t obj_idx, const uint8_t* name, uint16_t name_len) {
    Object* obj = &vm->objects[obj_idx];
    for (uint8_t i = 0; i < obj->attr_count; i++) {
        if (strncmp((char*)obj->attrs[i].name, (char*)name, name_len) == 0) {
            return obj->attrs[i].value;
        }
    }
    // Ако не е намерено в инстанцията, проверяваме класа
    if (obj->class_idx != 0) {
        return vm_get_attr(vm, obj->class_idx, name, name_len);
    }
    Value zero = {0};
    return zero;
}

// Управление на глобални променливи
void vm_set_global(VM* vm, const uint8_t* name, uint16_t name_len, Value value) {
    for (uint32_t i = 0; i < vm->global_count; i++) {
        if (strncmp((char*)vm->globals[i].name, (char*)name, name_len) == 0) {
            vm->globals[i].value = value;
            return;
        }
    }
    if (vm->global_count < MAX_GLOBALS) {
        strncpy((char*)vm->globals[vm->global_count].name, (char*)name, name_len < MAX_NAME_LENGTH ? name_len : MAX_NAME_LENGTH - 1);
        vm->globals[vm->global_count].value = value;
        vm->global_count++;
    }
}

Value vm_get_global(VM* vm, const uint8_t* name, uint16_t name_len) {
    for (uint32_t i = 0; i < vm->global_count; i++) {
        if (strncmp((char*)vm->globals[i].name, (char*)name, name_len) == 0) {
            return vm->globals[i].value;
        }
    }
    Value zero = {0};
    return zero;
}

// Изпълнение на байткод
void vm_run(VM* vm, uint8_t* bytecode, uint32_t size) {
    uint32_t pc = 0;

    while (pc < size) {
        uint8_t opcode = bytecode[pc++];
        uint16_t arg = bytecode[pc++] | (bytecode[pc++] << 8);

        switch (opcode) {
            case LOAD_CONST: {
                uint32_t offset = vm->bin->objects[arg].offset;
                if (vm->bin->objects[arg].type == 1) {  // Вложен байткод
                    vm->stack[vm->stack_top++].obj_idx = arg;
                } else if (vm->bin->objects[arg].type == 2) {
                    vm->stack[vm->stack_top++].i = *(int32_t*)(vm->bin->constants + offset);
                } else if (vm->bin->objects[arg].type == 4) {
                    vm->stack[vm->stack_top++].f = *(float*)(vm->bin->constants + offset);
                } else if (vm->bin->objects[arg].type == 3) {
                    vm->stack[vm->stack_top++].obj_idx = vm_create_object(vm, 0);  // Низ като обект (опростено)
                }
                break;
            }
            case STORE_FAST: {
                vm->locals[arg] = vm->stack[--vm->stack_top];
                break;
            }
            case LOAD_FAST: {
                vm->stack[vm->stack_top++] = vm->locals[arg];
                break;
            }
            case BINARY_ADD: {
                Value b = vm->stack[--vm->stack_top];
                Value a = vm->stack[--vm->stack_top];
                vm->stack[vm->stack_top].i = a.i + b.i;  // Само int за простота
                vm->stack_top++;
                break;
            }
            case LOAD_BUILD_CLASS: {
                uint32_t class_idx = vm_create_object(vm, 0);  // Нов клас без базов (за сега)
                vm->stack[vm->stack_top++].obj_idx = class_idx;
                break;
            }
            case MAKE_FUNCTION: {
                // Оставяме функцията на стека (obj_idx сочи към байткод)
                break;
            }
            case CALL_FUNCTION: {
                uint16_t argc = arg;
                Value func = vm->stack[--vm->stack_top];
                if (argc == 0) {  // Метод без аргументи
                    Value self = vm->stack[--vm->stack_top];
                    uint32_t bytecode_idx = func.obj_idx;
                    if (vm->bin->objects[bytecode_idx].type == 1) {
                        uint32_t offset = vm->bin->objects[bytecode_idx].offset;
                        vm->locals[0] = self;  // "self"
                        vm_run(vm, vm->bin->bytecode + offset, vm->bin->header.bytecode_size - offset);
                    }
                } else {  // Функция с аргументи
                    Value args[argc];
                    for (int i = argc - 1; i >= 0; i--) {
                        args[i] = vm->stack[--vm->stack_top];
                    }
                    uint32_t bytecode_idx = func.obj_idx;
                    if (vm->bin->objects[bytecode_idx].type == 1) {
                        uint32_t offset = vm->bin->objects[bytecode_idx].offset;
                        for (int i = 0; i < argc; i++) {
                            vm->locals[i] = args[i];
                        }
                        vm_run(vm, vm->bin->bytecode + offset, vm->bin->header.bytecode_size - offset);
                    }
                }
                break;
            }
            case STORE_NAME: {
                uint32_t offset = vm->bin->objects[arg].offset;
                uint16_t name_len = *(uint16_t*)(vm->bin->names + offset);
                Value value = vm->stack[--vm->stack_top];
                vm_set_global(vm, vm->bin->names + offset + 2, name_len, value);
                break;
            }
            case LOAD_NAME: {
                uint32_t offset = vm->bin->objects[arg].offset;
                uint16_t name_len = *(uint16_t*)(vm->bin->names + offset);
                vm->stack[vm->stack_top++] = vm_get_global(vm, vm->bin->names + offset + 2, name_len);
                break;
            }
            case STORE_ATTR: {
                uint32_t offset = vm->bin->objects[arg].offset;
                uint16_t name_len = *(uint16_t*)(vm->bin->names + offset);
                Value value = vm->stack[--vm->stack_top];
                Value obj = vm->stack[--vm->stack_top];
                vm_set_attr(vm, obj.obj_idx, vm->bin->names + offset + 2, name_len, value);
                break;
            }
            case LOAD_ATTR: {
                uint32_t offset = vm->bin->objects[arg].offset;
                uint16_t name_len = *(uint16_t*)(vm->bin->names + offset);
                Value obj = vm->stack[--vm->stack_top];
                vm->stack[vm->stack_top++] = vm_get_attr(vm, obj.obj_idx, vm->bin->names + offset + 2, name_len);
                break;
            }
            case RETURN_VALUE: {
                return;
            }
        }
    }
}

// Примерно зареждане и стартиране
void init_bin_file(BinFile* bin, uint8_t* data) {
    bin->header = *(BinHeader*)data;
    bin->objects = (ObjectEntry*)(data + 20);
    uint32_t table_size = bin->header.object_count * sizeof(ObjectEntry);
    bin->bytecode = data + 20 + table_size;
    bin->constants = bin->bytecode + bin->header.bytecode_size;
    bin->names = bin->constants + bin->header.constants_size;
    bin->varnames = bin->names + bin->header.names_size;
}

int main() {
    extern uint8_t bin_data[];  // BIN файл в паметта
    BinFile bin;
    init_bin_file(&bin, bin_data);

    VM vm;
    vm_init(&vm, &bin);

    // Изпълнение на първия байткод (главния модул)
    for (uint32_t i = 0; i < bin.header.object_count; i++) {
        if (bin.objects[i].type == 1) {
            vm_run(&vm, bin.bytecode + bin.objects[i].offset, bin.header.bytecode_size);
            break;
        }
    }

    // Примерен резултат
    if (vm.stack_top > 0) {
        printf("Final result: %d\n", vm.stack[vm.stack_top - 1].i);
    }

    return 0;
}