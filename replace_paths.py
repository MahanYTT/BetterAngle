import re
import glob

def repl(m):
    return 'GetAppStoragePath() + '

files = [
    'src/shared/ControlPanel.cpp',
    'src/main_app/BetterAngle.cpp',
    'src/config_tool/BetterAngleConfig.cpp',
    'src/shared/Updater.cpp'
]

for f in files:
    with open(f, 'r', encoding='utf-8') as file:
        data = file.read()
    
    # Needs to include State.h
    if '#include "shared/State.h"' not in data:
        data = '#include "shared/State.h"\n' + data

    data = data.replace('L"profiles/" + ', 'GetAppStoragePath() + ')
    data = data.replace('L"profiles"', 'GetAppStoragePath()')
    
    with open(f, 'w', encoding='utf-8') as file:
        file.write(data)

print("Done")
