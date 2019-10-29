#!/usr/bin/env bash
# Make sure we fail in case of errors
set -e

# FLAGS AND ENVIRONMENT:

TRAVIS_OS_NAME="unknown"
UPDATE_CONDA=false
TRAVIS_PYTHON_VERSION=2.7
TRAVIS_BUILD_NUMBER=1

LIBGFORTRAN_VERSION="3.0"
READLINE_VERSION="6.2"

ENVNAME=xsmodelsonly_test_$TRAVIS_PYTHON_VERSION


conda_channel=conda-forge/label/cf201901

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
echo "Python version: ${TRAVIS_PYTHON_VERSION}"


#conda config --remove channels conda-forge
#conda config --remove channels default


if $UPDATE_CONDA ; then
    # Updating conda
    echo "======================> Updating  conda..."
    conda update --yes -q conda conda-build
fi

if [[ ${TRAVIS_OS_NAME} == osx ]];
then
    conda config --add channels $conda_channel
fi

# Answer yes to all questions (non-interactive)
conda config --set always_yes true

# We will upload explicitly at the end, if successful
conda config --set anaconda_upload no

# Create test environment
echo " ======================>  Creating the test environment..."

conda create --yes --name $ENVNAME -c $conda_channel python=$TRAVIS_PYTHON_VERSION readline=${READLINE_VERSION}
#libgfortran=${LIBGFORTRAN_VERSION}

# Make sure conda-forge is the first channel
conda config --add channels $conda_channel

#conda config --add channels defaults

# Activate test environment
echo "=====================> Activate test environment..."

source $CONDA_PREFIX/etc/profile.d/conda.sh
conda activate $ENVNAME
#source activate $ENVNAME
echo "======> getting the file..."
if ! [ -f xspec-modelsonly-v6.22.1.tar.gz ]; then
    curl -LO -z xspec-modelsonly-v6.22.1.tar.gz https://heasarc.gsfc.nasa.gov/FTP/software/lheasoft/lheasoft6.22.1/xspec-modelsonly-v6.22.1.tar.gz
fi

# Build package
echo "Build package..."
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
    conda build --python=$TRAVIS_PYTHON_VERSION conda_recipe/xspec-modelsonly
    #conda index $HOME/miniconda/conda-bld
else
    # there is some strange error about the prefix length
    conda build --no-build-id --python=$TRAVIS_PYTHON_VERSION conda_recipe/xspec-modelsonly
    #conda index $HOME/miniconda/conda-bld
fi
echo "======> installing..."
conda install --use-local -c $conda_channel xspec-modelsonly

# Run tests
#cd astromodels/tests
#python -m pytest -vv --cov=astromodels # -k "not slow"

# Codecov needs to run in the main git repo

## Upload coverage measurements if we are on Linux
#if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then

#    echo "********************************** COVERAGE ******************************"
#    codecov -t 493c9a2d-42fc-40d6-8e65-24e681efaa1e#

#fi
