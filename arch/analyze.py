import subprocess
import os.path

# Requirements
path_to_analyze = os.path.join('..', 'src')
path_to_ignore = os.path.join('..', 'src', 'LiDAR')
save_path = os.path.join('requirements.txt')
subprocess.run(f"pipreqs {path_to_analyze} --ignore {path_to_ignore} --force --savepath {save_path}")

# tach mod
# tach sync
# tach show --web
