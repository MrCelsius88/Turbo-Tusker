rm -r build
mkdir -p build
pushd build

gcc -g ../src/cavern_linux_x.c -lX11 -o Cavern

popd
