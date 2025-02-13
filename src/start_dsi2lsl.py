import subprocess

exe_path = '../lib/dsi2lsl/dsi2lsl.exe'
args = ['port=COM3', 'lsl-stream-name=DSI7', 'montage=F4,C4,S1,S3,C3,F3']

result = subprocess.run([exe_path] + args, capture_output=True, text=True)

print(f"Output: {result.stdout}")
print(f"Error: {result.stderr}")