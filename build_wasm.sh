#bin/bash
set -e

micromamba create -n xeus-python-kernel --platform=emscripten-32 \
    -c https://repo.mamba.pm/emscripten-forge \
    -c https://repo.mamba.pm/conda-forge \
    --yes \
    python ipython pybind11 jedi xtl nlohmann_json pybind11_json numpy xeus xeus-python-shell

rm -rf build
mkdir build
cd build

export PREFIX=$MAMBA_ROOT_PREFIX/envs/xeus-python-kernel
export CMAKE_PREFIX_PATH=$PREFIX
export CMAKE_SYSTEM_PREFIX_PATH=$PREFIX

emcmake cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ON \
    -DCMAKE_PROJECT_INCLUDE=overwriteProp.cmake \
    -DXPYT_EMSCRIPTEN_WASM_BUILD=ON \
    ..

make -j5

emboa pack python core $MAMBA_ROOT_PREFIX/envs/xeus-python-kernel --version=3.10

cd ..

# patch output
python patch_it.py
