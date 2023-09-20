# Simple python script that build the project using the specified wgpu backend.
import argparse
import os
import platform
import subprocess

parser = argparse.ArgumentParser()

backendOptions = ["wgpu", "dawn"]
parser.add_argument("--backend", type=str,
    default=backendOptions[0],
    help="WebGPU backend to use (wgpu or dawn)"
)

parser.add_argument("--clean", action="store_true",
    default=False,
    help="If true, clean the build folder before building"
)

args = parser.parse_args()

backend = args.backend
if (backend not in backendOptions):
    print(f"⚠️  Invalid backend option \"{backend}\". Defaulting to {backendOptions[0]}.")
    backend = backendOptions[0]

buildFolder = f"build-{backend}"

if (args.clean):
    print(f"🧹 Cleaning build folder")
    subprocess.run(["rm", "-rf", buildFolder])

print(f"🏭 Create build files with {backend} backend")
subprocess.run(["cmake", ".", "-B", buildFolder, "-D", f"WEBGPU_BACKEND={backend.upper()}"])

print(f"🏗️ Building app for {platform.system()}")
subprocess.run(["cmake", "--build", buildFolder])

if platform.system() == "Windows":
    subprocess.run(os.path.join(buildFolder, "Debug", "LearnWebGPU.exe"))
else:
    subprocess.run(os.path.join(buildFolder, "LearnWebGPU"))