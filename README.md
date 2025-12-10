# hiat-tracking-baseboard

The image detection and tracking baseboard of HIAT

## download and build

```bash
git clone --recursive https://github.com/GhostLK/hiat-tracking-baseboard.git

cd hiat-tracking-baseboard && mkdir build && cd build

cmake .. && cmake --build .
```

## build and run test

```bash
cmake -DBUILD_TESTING=ON .. && cmake --build .
```
