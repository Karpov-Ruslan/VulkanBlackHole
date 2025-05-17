import subprocess
import os
import sys

shaders_input_dir = os.path.normpath(os.path.dirname(os.path.abspath(sys.argv[0])))
shaders_output_dir = shaders_input_dir + r"\spv"

shaders_list = {
    "black_hole_ray_marching_rk4.comp",
    "black_hole_ray_marching_rk2.comp",
    "black_hole_ray_marching_rk1.comp",
    "black_hole_precomputed.comp",
    "black_hole_precompute_precomputed_texture.comp"
}

for shader_name in shaders_list:
    try:
        subprocess.run(rf'glslc -O --target-env=vulkan1.0 -mfmt=c "{shaders_input_dir}\{shader_name}" -o "{shaders_output_dir}\{shader_name}.spv"', check=True)
    except:
        exit(1)