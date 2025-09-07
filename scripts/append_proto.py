import sys
import os
import re

def add_oneof(proto_file, oneof_name, new_field_type, new_field_name, target_messages):
    with open(proto_file, "r", encoding="utf-8") as f:
        text = f.read()

    for message_name in target_messages:
        pattern = rf"(message\s+{message_name}\s*\{{[\s\S]*?oneof\s+{oneof_name}\s*\{{\n)([\s\S]*?)(    \}}[\s\S]*?\n\}})"

        def replacer(match):
            message_start, oneof_body, message_end = match.groups()
            field_pattern = r"=\s*(\d+);"
            indices = [int(x) for x in re.findall(field_pattern, oneof_body)]
            next_index = max(indices) + 1 if indices else 1
            new_line = f"        {new_field_type}.{message_name} {new_field_name} = {next_index};\n"
            return message_start + oneof_body + new_line + message_end

        text = re.sub(pattern, replacer, text)

    with open(proto_file, "w", encoding="utf-8") as f:
        f.write(text)

def add_import(proto_file, import_path):
    with open(proto_file, "r", encoding="utf-8") as f:
        text = f.read()

    import_pattern = r'import\s+"[^"]+";'
    imports = list(re.finditer(import_pattern, text))

    if imports:
        last_import = imports[-1]
        new_import = f'\nimport "{import_path}";'
        text = text[:last_import.end()] + new_import + text[last_import.end():]
    else:
        # If no imports exist, add after package declaration
        package_pattern = r'(package\s+[^;]+;)'
        text = re.sub(package_pattern, rf'\1\n\nimport "{import_path}";', text)

    with open(proto_file, "w", encoding="utf-8") as f:
        f.write(text)


def parse_module_proto(proto_file):
    with open(proto_file, "r", encoding="utf-8") as f:
        text = f.read()

    pattern = r'//\s*([^.]+)\.([^\s\[]+)\s*\[([^\]]+)\]'
    match = re.search(pattern, text)

    if match:
        module_prefix = match.group(1)
        module_name = match.group(2)
        messages_str = match.group(3)
        messages = [msg.strip(' "') for msg in messages_str.split(',')]
        return f"{module_prefix}.{module_name}", module_name, messages

    return None, None, []

def main():
    if len(sys.argv) != 3:
        print("Usage: <bridge.proto_path> <module.proto_path>")
        sys.exit(1)
   
    bridge_proto_path = sys.argv[1]
    module_proto_path = sys.argv[2]
   
    if not os.path.exists(bridge_proto_path):
        print(f"Error: Proto file '{bridge_proto_path}' does not exist")
        sys.exit(1)
    else:
        print(f"Bridge main proto file: '{bridge_proto_path}'")
   
    if not os.path.exists(module_proto_path):
        print(f"Error: Config file '{module_proto_path}' does not exist")
        sys.exit(1)
    else:
        print(f"Bridge module proto file: '{module_proto_path}'")
   
    module, name, messages = parse_module_proto(module_proto_path)
    add_import(bridge_proto_path, os.path.basename(module_proto_path))
    add_oneof(bridge_proto_path, "subsystem", module, name, messages)

if __name__ == "__main__":
   main()