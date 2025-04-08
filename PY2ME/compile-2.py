import marshal
import struct
import types
import os
import sys
import glob

DEFAULT_PYC_DIR = "__pycache__"
BIN_FILE = "bytecode.bin"

# Типове обекти (съвместими с текущата VM)
OBJECT_TYPES = {
    'bytecode': 1,    # Байткод (code objects)
    'int': 2,         # 32-битов int
    'string': 3,      # ASCII низ
    'float': 4,       # 32-битов float
    'name': 5,        # Глобално име
    'varname': 6,     # Локална променлива
    'bool': 7,        # Булева стойност
    'list': 8,        # Списък
    'dict': 9,        # Речник
    'none': 10,       # None
    'builtin': 11     # Вградена функция (print, len, и т.н.)
}

class BinaryGenerator:
    def __init__(self):
        """Инициализира секциите и таблиците за линкване."""
        self.text_section = bytearray()      # .text - Байткод
        self.data_section = bytearray()      # .data - Прости константи
        self.rodata_section = bytearray()    # .rodata - Сложни константи (списъци, речници)
        self.symtab_section = bytearray()    # .symtab - Имена и локални променливи
        self.modtab_section = bytearray()    # .modtab - Таблица на модулите
        self.objects_table = []              # Таблица на обектите: тип, секция, офсет, module_id
        self.symbols_map = {}                # Карта за имена/локални: име -> индекс в objects_table
        self.constants_map = {}              # Карта за константи: стойност -> индекс в objects_table
        self.modules = {}                    # Име на модул -> module_id
        self.module_count = 0
        self.builtins = {'print': 0, 'len': 1, 'range': 2}  # Вградени функции с ID

    def add_to_section(self, section, data, obj_type, section_name, offset_map=None, module_id=None):
        """Добавя данни в секция и връща индекс в objects_table."""
        if offset_map is not None:
            key = str(data) + (f"_{module_id}" if module_id is not None else "")
            if key in offset_map:
                return offset_map[key]
            offset = len(section)
            idx = len(self.objects_table)
            offset_map[key] = idx
        else:
            offset = len(section)
            idx = len(self.objects_table)
        
        section.extend(data)
        self.objects_table.append((OBJECT_TYPES[obj_type], section_name, offset, module_id))
        return idx

    def process_bytecode(self, code_obj, module_id):
        """Добавя байткод в .text секцията."""
        self.add_to_section(self.text_section, code_obj.co_code, 'bytecode', 'text', module_id=module_id)

    def process_constant(self, const, module_id):
        """Обработва константи и ги разпределя в .data или .rodata."""
        if const is None:
            return self.add_to_section(self.data_section, b'', 'none', 'data', self.constants_map, module_id)
        elif isinstance(const, types.CodeType):
            return None  # Вложените code обекти се обработват отделно
        elif isinstance(const, int):
            data = struct.pack('<i', const)
            return self.add_to_section(self.data_section, data, 'int', 'data', self.constants_map, module_id)
        elif isinstance(const, float):
            data = struct.pack('<f', const)
            return self.add_to_section(self.data_section, data, 'float', 'data', self.constants_map, module_id)
        elif isinstance(const, str):
            ascii_str = const.encode('ascii', errors='ignore')
            data = struct.pack('<H', len(ascii_str)) + ascii_str
            return self.add_to_section(self.data_section, data, 'string', 'data', self.constants_map, module_id)
        elif isinstance(const, bool):
            data = struct.pack('<B', 1 if const else 0)
            return self.add_to_section(self.data_section, data, 'bool', 'data', self.constants_map, module_id)
        elif isinstance(const, list):
            data = bytearray()
            data.extend(struct.pack('<H', len(const)))  # Дължина на списъка
            for item in const:
                idx = self.process_constant(item, module_id)
                data.extend(struct.pack('<I', idx))  # Индекс на елемент
            return self.add_to_section(self.rodata_section, data, 'list', 'rodata', self.constants_map, module_id)
        elif isinstance(const, dict):
            data = bytearray()
            data.extend(struct.pack('<H', len(const)))  # Брой двойки
            for key, value in const.items():
                key_idx = self.process_constant(key, module_id)
                val_idx = self.process_constant(value, module_id)
                data.extend(struct.pack('<II', key_idx, val_idx))  # Индекси на ключ и стойност
            return self.add_to_section(self.rodata_section, data, 'dict', 'rodata', self.constants_map, module_id)
        elif isinstance(const, tuple):
            # Кортежите се третират като списъци за простота (може да се добави отделен тип)
            data = bytearray()
            data.extend(struct.pack('<H', len(const)))
            for item in const:
                idx = self.process_constant(item, module_id)
                data.extend(struct.pack('<I', idx))
            return self.add_to_section(self.rodata_section, data, 'list', 'rodata', self.constants_map, module_id)

    def process_name(self, name, module_id):
        """Добавя глобално име или builtin в .symtab или .data."""
        if name in self.builtins:
            data = struct.pack('<I', self.builtins[name])
            return self.add_to_section(self.data_section, data, 'builtin', 'data', self.symbols_map, module_id)
        ascii_name = name.encode('ascii', errors='ignore')
        data = struct.pack('<H', len(ascii_name)) + ascii_name
        return self.add_to_section(self.symtab_section, data, 'name', 'symtab', self.symbols_map, module_id)

    def process_varname(self, varname, module_id):
        """Добавя локална променлива в .symtab."""
        ascii_varname = varname.encode('ascii', errors='ignore')
        data = struct.pack('<H', len(ascii_varname)) + ascii_varname
        return self.add_to_section(self.symtab_section, data, 'varname', 'symtab', self.symbols_map, module_id)

    def process_code(self, code_obj, module_id):
        """Рекурсивно обработва code обект."""
        # Обработваме всички полета на code обекта
        self.process_bytecode(code_obj, module_id)
        
        # Константи (включително вложени code обекти)
        for const in code_obj.co_consts:
            self.process_constant(const, module_id)
        
        # Глобални имена (включително импорти и методи)
        for name in code_obj.co_names:
            self.process_name(name, module_id)
        
        # Локални променливи
        for varname in code_obj.co_varnames:
            self.process_varname(varname, module_id)
        
        # Free vars и cell vars (за closures)
        for freevar in code_obj.co_freevars:
            self.process_varname(freevar, module_id)
        for cellvar in code_obj.co_cellvars:
            self.process_varname(cellvar, module_id)
        
        # Рекурсивно обработваме вложени code обекти
        for const in code_obj.co_consts:
            if isinstance(const, types.CodeType):
                self.process_code(const, module_id)

    def process_module(self, pyc_path):
        """Обработва един .pyc файл като модул."""
        module_name = os.path.basename(pyc_path).split('.')[0]
        if module_name not in self.modules:
            self.modules[module_name] = self.module_count
            self.module_count += 1
            name_data = module_name.encode('ascii', errors='ignore')
            self.modtab_section.extend(struct.pack('<H', len(name_data)) + name_data)
        
        module_id = self.modules[module_name]
        code_obj = self.read_pyc(pyc_path)
        self.process_code(code_obj, module_id)

    def read_pyc(self, pyc_path):
        """Чете .pyc файла."""
        with open(pyc_path, 'rb') as f:
            f.read(16)  # Прескачане на хедъра (Python 3.x)
            return marshal.load(f)

    def write_bin(self, output_path):
        """Записва BIN файла със запазен формат за текущата VM."""
        with open(output_path, 'wb') as f:
            # Хедър
            header = struct.pack(
                '<IIIIII',
                len(self.objects_table),
                self.module_count,
                len(self.text_section),
                len(self.data_section),
                len(self.rodata_section),
                len(self.symtab_section) + len(self.modtab_section)
            )
            f.write(header)

            # Таблица на обектите
            for type_id, section_name, offset, module_id in self.objects_table:
                section_id = {'text': 1, 'data': 2, 'rodata': 3, 'symtab': 4}[section_name]
                module_id = module_id if module_id is not None else 0xFFFFFFFF  # -1 за глобални
                f.write(struct.pack('<IIII', type_id, section_id, offset, module_id))

            # Секции
            f.write(self.text_section)
            f.write(self.data_section)
            f.write(self.rodata_section)
            f.write(self.symtab_section)
            f.write(self.modtab_section)

    def generate_bin_from_directory(self, pyc_dir=DEFAULT_PYC_DIR, output_path=BIN_FILE):
        """Генерира BIN от всички .pyc файлове в директория."""
        pyc_files = glob.glob(os.path.join(pyc_dir, "*.pyc"))
        if not pyc_files:
            print(f"Error: No .pyc files found in {pyc_dir}")
            return

        print(f"Found {len(pyc_files)} .pyc files: {pyc_files}")
        for pyc_path in pyc_files:
            print(f"Processing {pyc_path}...")
            self.process_module(pyc_path)

        self.write_bin(output_path)
        print(f"Generated BIN file: {output_path}")
        print(f"Modules: {self.module_count}")
        print(f"Objects: {len(self.objects_table)}")
        print(f"Text size: {len(self.text_section)} bytes")
        print(f"Data size: {len(self.data_section)} bytes")
        print(f"Rodata size: {len(self.rodata_section)} bytes")
        print(f"Symtab size: {len(self.symtab_section) + len(self.modtab_section)} bytes")

def main():
    pyc_dir = DEFAULT_PYC_DIR
    bin_file = BIN_FILE
    if len(sys.argv) > 1:
        pyc_dir = sys.argv[1]
    if len(sys.argv) > 2:
        bin_file = sys.argv[2]

    generator = BinaryGenerator()
    generator.generate_bin_from_directory(pyc_dir, bin_file)

if __name__ == "__main__":
    main()