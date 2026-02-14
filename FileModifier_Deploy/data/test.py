import os
import random

# 1. doc.txt
with open('doc.txt', 'w', encoding='utf-8') as f:
    f.write('Hello World')

# 2. data.bin – 10000 байт случайных данных
with open('data.bin', 'wb') as f:
    f.write(os.urandom(10000))

# 3. readme (без расширения)
with open('readme', 'w', encoding='utf-8') as f:
    f.write('No extension')

# 4. sub/deep.txt
os.makedirs('sub', exist_ok=True)
with open('sub/deep.txt', 'w', encoding='utf-8') as f:
    f.write('Recursive test')

# 5. conflict.txt (source)
with open('conflict.txt', 'w', encoding='utf-8') as f:
    f.write('Original')

# 6. target/conflict.txt
os.makedirs('target', exist_ok=True)
with open('target/conflict.txt', 'w', encoding='utf-8') as f:
    f.write('Existing target file')

# 7. target_rename/conflict.txt, conflict_1.txt, conflict_2.txt
os.makedirs('target_rename', exist_ok=True)
with open('target_rename/conflict.txt', 'w', encoding='utf-8') as f:
    f.write('Any text for conflict.txt')
with open('target_rename/conflict_1.txt', 'w', encoding='utf-8') as f:
    f.write('Any text for conflict_1.txt')
with open('target_rename/conflict_2.txt', 'w', encoding='utf-8') as f:
    f.write('Any text for conflict_2.txt')

print("Все файлы успешно созданы.")
