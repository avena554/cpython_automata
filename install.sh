#!/bin/sh

echo "Fetching PyPA's build if needed" 
python3 -m pip install --upgrade build
echo "Building Pyta lib"
cd python
python3 -m build
echo "Installing"
python3 -m pip install dist/pyta-*.whl
cd ..
echo "Building and installing C extension"
make -C c_backend install





