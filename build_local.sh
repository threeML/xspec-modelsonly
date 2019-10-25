#!/usr/bin/env bash
TRAVIS_OS_NAME="unknown"

if [[ "$OSTYPE" == "linux-gnu" ]]; then

        # Linux

        TRAVIS_OS_NAME="linux"


elif [[ "$OSTYPE" == darwin* ]]; then

        # Mac OSX

        TRAVIS_OS_NAME="osx"


elif [[ "$OSTYPE" == "cygwin" ]]; then

        # POSIX compatibility layer and Linux environment emulation for Windows

        TRAVIS_OS_NAME="linux"

else

        # Unknown.

        echo "Could not guess your OS. Exiting."

        exit 1

fi

echo "Running on ${TRAVIS_OS_NAME}"

TRAVIS_PYTHON_VERSION=2.7
TRAVIS_BUILD_NUMBER=0

ENVNAME=xsmodelsonly_test_$TRAVIS_PYTHON_VERSION

echo "Python version: ${TRAVIS_PYTHON_VERSION}"

# Make sure we fail in case of errors
set -e

# Environment
libgfortranver="3.0"

UPDATE_CONDA=true

#if [[ ${TRAVIS_OS_NAME} == linux ]];
#then
#    miniconda_os=Linux
#    compilers="gcc_linux-64 gxx_linux-64 gfortran_linux-64"
#else  # osx
#    miniconda_os=MacOSX
#    compilers="clang_osx-64 clangxx_osx-64 gfortran_osx-64"
#   # On macOS we also need the conda libx11 libraries used to build xspec
#    # We also need to pin down ncurses, for now only on macos.
#    xorg="xorg-libx11 ncurses=5"
#fi



if $UPDATE_CONDA ; then
    # Update conda
    echo "Update conda..."
    conda update --yes -q conda conda-build
fi

if [[ ${TRAVIS_OS_NAME} == osx ]];
then
    conda config --add channels conda-forge
fi

# Answer yes to all questions (non-interactive)
conda config --set always_yes true

# We will upload explicitly at the end, if successful
conda config --set anaconda_upload no

# Create test environment
echo "Create test environment..."

conda create --yes --name $ENVNAME -c conda-forge python=$TRAVIS_PYTHON_VERSION libgfortran=${libgfortranver}

# Make sure conda-forge is the first channel
conda config --add channels conda-forge

conda config --add channels defaults

# Activate test environment
echo "Activate test environment..."

source $CONDA_PREFIX/etc/profile.d/conda.sh
conda activate $ENVNAME
#source activate $ENVNAME

# Build package
echo "Build package..."
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
    conda build --python=$TRAVIS_PYTHON_VERSION conda_recipe/xspec-modelsonly
    #conda index $HOME/work/fermi/miniconda3/conda-bld
    conda index $HOME/miniconda/conda-bld
else
    # there is some strange error about the prefix length
    conda build --no-build-id --python=$TRAVIS_PYTHON_VERSION conda_recipe/xspec-modelsonly
    conda index $HOME/miniconda/conda-bld
fi
echo "======> installing..."
conda install --use-local -c conda-forge xspec-modelsonly

# Run tests
#cd astromodels/tests
#python -m pytest -vv --cov=astromodels # -k "not slow"

# Codecov needs to run in the main git repo

## Upload coverage measurements if we are on Linux
#if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

#    echo "********************************** COVERAGE ******************************"
#    codecov -t 493c9a2d-42fc-40d6-8e65-24e681efaa1e#

#fi
