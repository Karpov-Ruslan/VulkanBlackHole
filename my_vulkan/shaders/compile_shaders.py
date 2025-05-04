import subprocess
import os
import sys

shaders_input_dir = os.path.normpath(os.path.dirname(os.path.abspath(sys.argv[0])))
shaders_output_dir = shaders_input_dir + r"\spv"

shaders_list = {
    "black_hole.comp",
}

for shader_name in shaders_list:
    try:
        subprocess.run(rf'glslc -O --target-env=vulkan1.2 -mfmt=c "{shaders_input_dir}\{shader_name}" -o "{shaders_output_dir}\{shader_name}.spv"', check=True)
    except:
        exit(1)