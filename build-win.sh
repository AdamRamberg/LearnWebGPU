# Create build files
cmake . -B build -DWEBGPU_BACKEND=DAWN
# Build program
cmake --build build

build\Debug\LearnWebGPU.exe  # Windows