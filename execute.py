import os

os.system('cd build && make && ./ssc')
os.system('arch -x86_64 gcc build/test.s -o build/test')