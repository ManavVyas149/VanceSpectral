import os

fonts = [
    ('JetBrainsMonoBold_ttf', 'Resources/Fonts/JetBrainsMono-Bold.ttf'),
    ('JetBrainsMonoRegular_ttf', 'Resources/Fonts/JetBrainsMono-Regular.ttf'),
    ('SpaceGrotesk_ttf', 'Resources/Fonts/SpaceGrotesk.ttf')
]

header_content = ['#pragma once\n\nnamespace BinaryData\n{\n']
cpp_content = ['#include "BinaryFontData.h"\n\nnamespace BinaryData\n{\n']

for var_name, file_path in fonts:
    with open(file_path, 'rb') as f:
        data = f.read()
    size = len(data)
    header_content.append(f'    extern const char* {var_name};\n')
    header_content.append(f'    const int {var_name}Size = {size};\n\n')
    
    arr_name = var_name + '_bytes'
    cpp_content.append(f'    static const unsigned char {arr_name}[] = {{\n')
    
    lines = []
    for i in range(0, size, 20):
        chunk = data[i:i+20]
        lines.append('        ' + ', '.join(f'0x{b:02x}' for b in chunk))
    cpp_content.append(',\n'.join(lines))
    cpp_content.append('\n    };\n')
    cpp_content.append(f'    const char* {var_name} = (const char*) {arr_name};\n\n')

header_content.append('}\n')
cpp_content.append('}\n')

with open('Source/BinaryFontData.h', 'w') as f:
    f.writelines(header_content)
    
with open('Source/BinaryFontData.cpp', 'w') as f:
    f.writelines(cpp_content)

print('Generated Source/BinaryFontData.h and Source/BinaryFontData.cpp successfully!')
