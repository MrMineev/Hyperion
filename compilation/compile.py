import json
import os

path = "compilation/compile_command.json"

data = json.load(open(path, 'r'))

command = data['main'] + ' '
for filepath in data['programs']:
    command += filepath + ' '
for filepath in data['modules']:
    command += filepath + ' '
command = f"{command} -o {data['output']}"

print(command)

with open("compilation/compile.sh", "w") as file:
    file.write(
        f"#!/bin/bash\n\n{command}"
    ) 
    file.close()

os.system("chmod +x compilation/compile.sh")
os.system("./compilation/compile.sh")
