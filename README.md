# README

## Dependencies:

* eigen 3
* boost-filesystem
* cmake 
* OpenNI2
* cuda

for OpenNI2, in a directory of your own choosing:
```bash
git clone https://github.com/OpenNI/OpenNI2.git
cd OpenNI2
make
```
see www.openni.org 

## Building

Building the project **should** then be as simple as executing the following commands (from the project directory)

```bash
mkdir build
cd build
cmake ..
make
```

If you are using GCC greater than 5.4 you will get an error, since nvcc does not currently support any version higher than that

The sdf_tracker_app is a quite minimal example of the large-scale SDF_tracker in use and uses OpenNI2 to capture depth images.