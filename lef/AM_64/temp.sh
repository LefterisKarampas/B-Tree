if [ -f ./lef ]; then
    rm lef
fi
make main
if [ "$?" != "0" ];then	
	echo "Fail Compile!"
	exit 1
fi
./build/main
