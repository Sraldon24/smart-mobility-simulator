cd backend/build
sed -i 's/8301/8302/g' ../src/main.cpp
make
strace -f -e network ./smart_mobility_backend > strace.log 2>&1
