#!/usr/bin/env bash
# Make sure we fail in case of errors
set -e
# FROM TRAVIS:
TRAVIS_OS_NAME="unknown"
TRAVIS_PYTHON_VERSION=3.7
TRAVIS_BUILD_NUMBER=1
TRAVIS_EVENT_TYPE="push"
#TRAVIS_BRANCH="master"
TRAVIS_BRANCH="no_master"
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



# Build and test...
# FLAGS AND ENVIRONMENT:
LIBGFORTRAN_VERSION="3.0"
READLINE_VERSION="6.2"
UPDATE_CONDA=false

ENVNAME=xsmodelsonly_test_$TRAVIS_PYTHON_VERSION

conda_channel=conda-forge

echo "Running on ${TRAVIS_OS_NAME}"
echo "Python version: ${TRAVIS_PYTHON_VERSION}"

if $UPDATE_CONDA ; then
    # Updating conda
    echo "======================> Updating  conda..."
    conda update --yes -q conda conda-build
fi

if [[ ${TRAVIS_PYTHON_VERSION} == 2.7 ]];
then
    READLINE="readline=${READLINE_VERSION}"
fi

# Answer yes to all questions (non-interactive)
conda config --set always_yes true

# We will upload explicitly at the end, if successful
conda config --set anaconda_upload no

# Create test environment
echo " ======================>  Creating the test environment..."

conda create --yes --name $ENVNAME -c $conda_channel python=$TRAVIS_PYTHON_VERSION ${READLINE}
#libgfortran=${LIBGFORTRAN_VERSION}

# Make sure conda-forge is the first channel
conda config --add channels $conda_channel

#conda config --add channels defaults

# Activate test environment
echo "=====================> Activate test environment..."

#source $CONDA_PREFIX/etc/profile.d/conda.sh
source /home/ndilalla/work/fermi/miniconda3/etc/profile.d/conda.sh
conda activate $ENVNAME

echo "======> getting the file..."
#if ! [ -f xspec-modelsonly-v6.22.1.tar.gz ]; then
#    curl -LO -z xspec-modelsonly-v6.22.1.tar.gz https://heasarc.gsfc.nasa.gov/FTP/software/lheasoft/lheasoft6.22.1/xspec-modelsonly-v6.22.1.tar.gz
#fi

# Build package
echo "Build package..."

#conda install conda-verify

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

# UPLOAD TO CONDA:
# If we are on the master branch upload to the channel
# if [[ "${TRAVIS_EVENT_TYPE}" == "pull_request" ]]; then
#     echo "This is a pull request, not uploading to Conda channel"
# else
#     if [[ "${TRAVIS_EVENT_TYPE}" == "push" ]]; then
#         echo "This is a push to TRAVIS_BRANCH=${TRAVIS_BRANCH}"
#         if [[ "${TRAVIS_BRANCH}" == "master" ]]; then
#             conda install -c conda-forge anaconda-client
#             echo "Uploading ${CONDA_BUILD_PATH}"
#             if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
#                 anaconda -t $CONDA_UPLOAD_TOKEN upload -u omodei $HOME/miniconda/conda-bld/linux-64/*.tar.bz2 --force
#             else
#                 anaconda -t $CONDA_UPLOAD_TOKEN upload -u omodei $HOME/miniconda/conda-bld/*/*.tar.bz2 --force
#             fi
#         fi
#     fi
# fi
