
import marshal
import struct
import types
import os
import sys
import glob

# Глобални променливи
DEFAULT_PYC_DIR = "wiz/__pycache__"  # Дефолтна директория за .pyc файлове
BIN_FILE = "wiz/__pycache__/bytecode.bin"        # Изходен BIN файл

# Константи за типове обекти
OBJECT_TYPES = {
    'bytecode': 1,  # Суров байткод от co_code
    'int': 2,       # Цяло число (32-bit)
    'string': 3,    # Низ (ASCII)
    'float': 4,     # Число с плаваща запетая (32-bit)
    'name': 5,      # Глобално име от co_names
    'varname': 6    # Локална променлива от co_varnames
}

class BinaryGenerator:
    def __init__(self):
        """Инициализира секциите и таблиците за BIN файла."""
        self.bytecode_section = bytearray()
        self.constants_section = bytearray()
        self.names_section = bytearray()
        self.varnames_section = bytearray()
        self.objects_table = []
        self.constants_map = {}
        self.names_map = {}
        self.varnames_map = {}

    def add_to_section(self, section, data, obj_type, offset_map=None):
        """
        Добавя данни в секция и записва в таблицата на обектите.
        Проверява за дубликати, ако е предоставен offset_map.
        """
        if offset_map is not None:
            key = str(data)
            if key in offset_map:
                return offset_map[key]
            offset = len(section)
            offset_map[key] = offset
        else:
            offset = len(section)

        section.extend(data)
        self.objects_table.append((OBJECT_TYPES[obj_type], offset))
        return offset

    def process_bytecode(self, code_obj):
        """Обработва суровия байткод от code обект."""
        self.add_to_section(self.bytecode_section, code_obj.co_code, 'bytecode')

    def process_constant(self, const):
        """Обработва константа (int, string, float)."""
        if const is None or isinstance(const, types.CodeType):
            return

        if isinstance(const, int):
            data = struct.pack('<i', const)
            self.add_to_section(self.constants_section, data, 'int', self.constants_map)
        elif isinstance(const, str):
            ascii_str = const.encode('ascii', errors='ignore')
            data = struct.pack('<H', len(ascii_str)) + ascii_str
            self.add_to_section(self.constants_section, data, 'string', self.constants_map)
        elif isinstance(const, float):
            data = struct.pack('<f', const)
            self.add_to_section(self.constants_section, data, 'float', self.constants_map)

    def process_name(self, name):
        """Обработва глобално име от co_names."""
        ascii_name = name.encode('ascii', errors='ignore')
        data = struct.pack('<H', len(ascii_name)) + ascii_name
        self.add_to_section(self.names_section, data, 'name', self.names_map)

    def process_varname(self, varname):
        """Обработва локална променлива от co_varnames."""
        ascii_varname = varname.encode('ascii', errors='ignore')
        data = struct.pack('<H', len(ascii_varname)) + ascii_varname
        self.add_to_section(self.varnames_section, data, 'varname', self.varnames_map)

    def process_code(self, code_obj):
        """Рекурсивно обработва code обект."""
        self.process_bytecode(code_obj)
        for const in code_obj.co_consts:
            self.process_constant(const)
        for name in code_obj.co_names:
            self.process_name(name)
        for varname in code_obj.co_varnames:
            self.process_varname(varname)
        for const in code_obj.co_consts:
            if isinstance(const, types.CodeType):
                self.process_code(const)

    def read_pyc(self, pyc_path):
        """Чете .pyc файла и връща главния code обект."""
        with open(pyc_path, 'rb') as f:
            f.read(16)  # Прескачане на хедъра
            return marshal.load(f)

    def write_bin(self, output_path):
        """Записва всички секции в BIN файл."""
        with open(output_path, 'wb') as f:
            header = struct.pack(
                '<IIIII',
                len(self.objects_table),
                len(self.bytecode_section),
                len(self.constants_section),
                len(self.names_section),
                len(self.varnames_section)
            )
            f.write(header)
            for type_id, offset in self.objects_table:
                f.write(struct.pack('<II', type_id, offset))
            f.write(self.bytecode_section)
            f.write(self.constants_section)
            f.write(self.names_section)
            f.write(self.varnames_section)

    def generate_bin_from_directory(self, pyc_dir=DEFAULT_PYC_DIR, output_path=BIN_FILE):
        """
        Генерира BIN файл от всички .pyc файлове в указаната директория.
        Ако има няколко .pyc файла, обработва ги последователно.
        """
        # Намиране на всички .pyc файлове
        pyc_files = glob.glob(os.path.join(pyc_dir, "*.pyc"))
        if not pyc_files:
            print(f"Error: No .pyc files found in {pyc_dir}")
            return

        print(f"Found {len(pyc_files)} .pyc files in {pyc_dir}: {pyc_files}")

        # Обработка на всеки .pyc файл
        for pyc_path in pyc_files:
            print(f"Processing {pyc_path}...")
            code_obj = self.read_pyc(pyc_path)
            self.process_code(code_obj)

        # Запис на BIN файла
        self.write_bin(output_path)

        # Информация за дебъг
        print(f"Generated BIN file: {output_path}")
        print(f"Objects: {len(self.objects_table)}")
        print(f"Bytecode size: {len(self.bytecode_section)} bytes")
        print(f"Constants size: {len(self.constants_section)} bytes")
        print(f"Names size: {len(self.names_section)} bytes")
        print(f"Varnames size: {len(self.varnames_section)} bytes")

def main():
    """Главна функция с поддръжка на аргументи от командния ред."""
    pyc_dir = DEFAULT_PYC_DIR
    bin_file = BIN_FILE

    # Проверка на аргументи от командния ред
    if len(sys.argv) > 1:
        pyc_dir = sys.argv[1]
    if len(sys.argv) > 2:
        bin_file = sys.argv[2]

    generator = BinaryGenerator()
    generator.generate_bin_from_directory(pyc_dir, bin_file)

if __name__ == "__main__":
    main()