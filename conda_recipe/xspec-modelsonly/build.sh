echo "================="
echo $PWD
echo $RECIPE_DIR

tar xf heasoft-6.30.1src.tar.gz

CWD="$PWD"
XSPEC_PATCH="Xspatch_121201c.tar.gz"
XSPEC_PATCH_INSTALLER="patch_install_4.15.tcl"
XSPEC_MODELS_ONLY=heasoft-6.30.1

# If a patch is required, download the necessary file and apply it
if [ -n "$XSPEC_PATCH" ]
then
    cd ${XSPEC_MODELS_ONLY}/Xspec/src;
    curl -LO -z ${XSPEC_PATCH} https://heasarc.gsfc.nasa.gov/docs/xanadu/xspec/issues/archive/${XSPEC_PATCH};
    curl -LO -z ${XSPEC_PATCH_INSTALLER} https://heasarc.gsfc.nasa.gov/docs/xanadu/xspec/issues/archive/${XSPEC_PATCH_INSTALLER};
    tclsh ${XSPEC_PATCH_INSTALLER} -m -n;
    #rm -rf XSFits;
    cd ${CWD};
fi

#Copy in the OGIPTable fix
#cp $RECIPE_DIR/../../OGIPTable.cxx heasoft-6.25/Xspec/src/XSModel/Model/Component/OGIPTable

cd ${XSPEC_MODELS_ONLY}/BUILD_DIR

# We need a custom include and library path to use the packages installed
# in the build environment

export CFLAGS="-I$CONDA_PREFIX/include"
export CXXFLAGS="-std=c++11 -Wno-c++11-narrowing -Wall -Wno-deprecated -I$CONDA_PREFIX/include"
export LDFLAGS="$LDFLAGS -L$CONDA_PREFIX/lib"
# -L${PREFIX}/lib"

# Patch the configure script so XSModel is built
sed -i.orig "s|src/XSFunctions|src/XSFunctions src/XSModel|g" configure
#sed -i.orig "s|wcslib cfitsio CCfits heasp|cfitsio heasp|g" configure

if [ "$(uname)" == "Linux" ]; then
    ./configure --prefix=${SRC_DIR}/xspec-modelsonly-build --enable-xs-models-only --disable-x
    #./hmake '--quiet' 'XSLM_USER_FLAGS="-I${PREFIX}/include"' 'XSLM_USER_LIBS="-L${PREFIX}/lib -lCCfits -lcfitsio -lwcslib -lgfortran"'
    make HD_ADD_SHLIB_LIBS=yes
fi

if [ "$(uname)" == "Darwin" ]; then
    # Build for a fairly old mac to ensure portability
    ./configure --prefix=${SRC_DIR}/xspec-modelsonly-build --enable-xs-models-only --disable-x
    #./hmake '--quiet' 'LDFLAGS_CXX=-headerpad_max_install_names -lcfitsio -lCCfits -lccfits -lwcs -lgfortran' 'XSLM_USER_LIBS="-L${PREFIX}/lib -lCCfits -lcfitsio -lwcslib -lgfortran"'
    make HD_ADD_SHLIB_LIBS=yes
fi

make install

# Correct the output of make install so that the output directory structure
# makes more sense for a conda environment. We will place libraries in the
# ${PREFIX}/lib directory and data files in ${PREFIX}/Xspec/spectral

# Copy libraries in the ${PREFIX}/lib directory

#cp -v `find ${SRC_DIR}/xspec-modelsonly-build/Xspec/ -name "libXS*"` ${PREFIX}/lib
mkdir -p ${PREFIX}/lib
if [ "`uname -s`" = "Linux" ] ; then
  cp -L ${SRC_DIR}/xspec-modelsonly-build/x86*/lib/*.so* ${PREFIX}/lib/
else
  cp -L ${SRC_DIR}/xspec-modelsonly-build/x86*/lib/*.dylib* ${PREFIX}/lib/
fi
cp -L ${SRC_DIR}/xspec-modelsonly-build/x86*/lib/*.a ${PREFIX}/lib/

# Create a ${PREFIX}/lib/Xspec directory
mkdir ${PREFIX}/lib/Xspec

# Create an empty headas directory. The env. variable HEADAS should point here at
# runtime
mkdir ${PREFIX}/lib/Xspec/headas

# Fill it with a useless file, otherwise Conda will remove it during installation
echo "LEAVE IT HERE" > ${PREFIX}/lib/Xspec/headas/DO_NOT_REMOVE

# Copy the spectral data in the right place. According to the Xspec documentation,
# this should be $HEADAS/../spectral
cp -rv ${SRC_DIR}/xspec-modelsonly-build/spectral ${PREFIX}/lib/Xspec

mkdir -p ${PREFIX}/include
mkdir -p ${PREFIX}/include/XSFunctions
mkdir -p ${PREFIX}/include/XSFunctions/Utilities

cp -L ${SRC_DIR}/xspec-modelsonly-build/Xspec/x86*/include/XSFunctions/Utilities/xsFortran.h ${PREFIX}/include/XSFunctions/Utilities
cp -L ${SRC_DIR}/xspec-modelsonly-build/x86*/include/xsTypes.h ${PREFIX}/include
cp -L ${SRC_DIR}/xspec-modelsonly-build/Xspec/x86*/include/XSFunctions/funcWrappers.h ${PREFIX}/include/XSFunctions
