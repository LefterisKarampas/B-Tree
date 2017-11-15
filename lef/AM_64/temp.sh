if [ -f ./lef ]; then
    rm lef
fi
make main
./build/main
