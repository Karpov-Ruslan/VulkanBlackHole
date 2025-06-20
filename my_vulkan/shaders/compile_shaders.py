import subprocess
import os
import sys

shaders_input_dir = os.path.normpath(os.path.dirname(os.path.abspath(sys.argv[0])))
shaders_output_dir = shaders_input_dir + r"//spv"

shaders_list = (
    ("black_hole_ray_marching_rk4.comp", "vulkan1.0"),
    ("black_hole_ray_marching_rk2.comp", "vulkan1.0"),
    ("black_hole_ray_marching_rk1.comp", "vulkan1.0"),
    ("black_hole_ray_query.comp", "vulkan1.2")
)

for shader_name, vulkan_env in shaders_list:
    try:
        subprocess.run(rf'glslc -O --target-env={vulkan_env} -mfmt=c "{shaders_input_dir}//{shader_name}" -o "{shaders_output_dir}//{shader_name}.spv"', check=True)
    except:
        exit(1)