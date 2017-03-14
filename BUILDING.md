# Building Region-Templates.js

## Preparing workspace

Assuming: 
- You have no previous dependency installed
- ActiveHarmony [4.6.0](http://www.dyninst.org/sites/default/files/downloads/harmony/ah-4.6.0.tar.gz)
- OpenCV [2.4.9](http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/2.4.9/opencv-2.4.9.zip/download)
- InsightToolkit [4.11.0](https://sourceforge.net/projects/itk/files/itk/4.11/InsightToolkit-4.11.0.tar.gz/download)
- Nscale cloned on commit [#daac7ff](https://github.com/SBU-BMI/nscale/commit/daac7ff1be5198726e9225a6d4bdd97216be8e42)
- Yi provided on [region-templates/runtime/regiontemplates/external-src/](https://github.com/SBU-BMI/region-templates/tree/master/runtime/regiontemplates/external-src)


for better organization, your workspace should look like 
```
    /workspace
        - /libs
            - /ah
                - /activeharmony-4.6.0
            - hadoopgis
            - /itk
                - /InsightToolkit-4.11.0
                - /build
            - /ns
                - /nscale
                - /build
            - /opencv
                - /opencv-2.4.9
                - /build
            - /yi
                - /yi-src
                - /build
        - /project
            - /region-templates
            - build
        - /imgs
```


after everythins is downloaded, clone the repository into `/project` folder

## Installing Opencv 
Requirements as noted on the [Opencv install guide](http://docs.opencv.org/2.4/doc/tutorials/introduction/linux_install/linux_install.html)


compiler
```
sudo apt-get install build-essential
```

required
```
sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
```

optional
```
sudo apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev
```

download file and move to the `/opencv` folder

### To build and install

inside `/opencv` folder
```
unzip opencv-2.4.9.zip
cd build
cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local ../opencv-2.4.9/
make
sudo make install
```

## Installing InsightToolkit

download file and move to the `/itk` folder

### To build and install

inside `/itk` folder
```
tar -vxzf InsightToolkit-4.11.0.tar.gz
cd build
ccmake ../InsightToolkit-4.11.0
```

on the interface
- press `c` to configure
- press `t` to toggle advanced mode
- turn on `BUILD_SHARED_LIBS`
- turn on `MODULE_ITKVideoBridgeOpenCV`
- press `c` to configure
- press `g` to generate

then build
```
make
```

## Installing Nscale

clone the repository inside `/ns` folder

### To build and install

inside `/ns` folder
```
cd nscale
git checkout daac7ff
cd ../build
ccmake ../nscale
```

on the interface
- press `c` to configure
- turn on `NS_SEGMENT`
- turn on `NS_NORMALIZATION`
- turn on `NS_FEATURE`
- press `c` to configure
- press `g` to generate

then build
```
make
```

NOTE: it may give errors of vars `NSCALE_VERSION_MAJOR` and `NSCALE_VERSION_MINOR`, press `e` to continue and ignore.

## Installing Yi's library

inside `/workspace` folder
```
cp project/region-templates/runtime/regiontemplates/external-src/yi-src.tar.gz ./libs/yi/
```

### To build and install

inside `/yi` folder

```
tar -vxzf yi-src.tar.gz
cd build
ccmake ../yi-src
```

on the interface
- press `c` to configure
- press `e` to ignore errors and continue
- fill `ITK_DIR` to your `ITK` installation
- turn off `build_mainTileAndSegmentWSINuclei`
- press `c` to configure
- turn on `build_mainSegmentSmallImage`
- press `c` to configure
- press `g` to generate

then build
```
make
```

## Installing Active Harmony

download file and move to the `/ah` folder

### To build and install

inside `/ah` folder
```
tar -vxzf ah-4.6.0.tar.gz
cd activeharmony-4.6.0 
make install
```

## Installing Hadoop-GIS

inside `/workspace` folder
```
cp project/region-templates/runtime/regiontemplates/external-src/hadoopgis/ ./libs/
```

### To build and install

inside `/hadoopgis` folder

```
cd installer
sudo bash installhadoopgis.sh 
```

NOTE: it builds to `/build` folder inside `/hadoopgis`

## Installing Region-Templates

### To build

inside `/project` folder
```
cd build
ccmake ../region-templates/runtime
```

on the interface
- press `c` to configure
- turn on `BUILD_SAMPLE_APPLICATIONS`
- turn on `RTEMPLATES_EXAMPLE`
- turn on `USE_REGION_TEMPLATES`
- press `c` to configure
- press `e` to ignore errors and continue
- fill `NSCALE_BUILD_DIR` to your `NSCALE` build
- fill `NSCALE_SRC_DIR` to your `NSCALE` source
- press `c` to configure
- turn on `RT_TUNING_NSCALE_EXAMPLE`
- turn on `RT_TUNING_YI_EXAMPLE`
- turn on `USE_ACTIVE_HARMONY`
- turn on `USE_HADOOPGIS`
- press `c` to configure
- press `e` to ignore errors and continue
- fill `AH_SRC_DIR` to your `active harmony` source
- fill `HADOOPGIS_BUILD_DIR` to your `hadoopgis` build
- fill `ITK_DIR` to your `ITK` installation
- fill `YI_BUILD_DIR` to your `YI` build
- fill `YI_SRC_DIR` to your `YI` source
- press `c` to configure
- press `g` to generate

then build
```
make
```
