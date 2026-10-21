#!/bin/bash

echo "Setup MacOS environment for run lupdate"
trap 'echo Setup failed; exit 1' ERR

#  Go one-up from MuseScore root dir
cd ..

# Let's remove the file with environment variables to recreate it
ENV_FILE=./musescore_lupdate_environment.sh
rm -f ${ENV_FILE}

echo "echo 'Setup MacOS environment for run lupdate'" >> ${ENV_FILE}

export QT_SHORT_VERSION=5.15.9
export QT_PATH=$HOME/Qt
export QT_MACOS=$QT_PATH/$QT_SHORT_VERSION/clang_64
export PATH=$PATH:$QT_MACOS/bin
wget -nv -O qt5.zip https://s3.amazonaws.com/utils.musescore.org/Qt5159_mac.zip
mkdir -p $QT_MACOS
unzip -qq qt5.zip -d $QT_MACOS
rm qt5.zip

echo export PATH="${QT_MACOS}/bin:\${PATH}" >> ${ENV_FILE}
echo export LD_LIBRARY_PATH="${QT_PATH}/lib:\${LD_LIBRARY_PATH}" >> ${ENV_FILE}
echo export QT_PATH="${QT_PATH}" >> ${ENV_FILE}
echo export QT_PLUGIN_PATH="${QT_PATH}/plugins" >> ${ENV_FILE}
echo export QML2_IMPORT_PATH="${QT_PATH}/qml" >> ${ENV_FILE}

echo "Setup script done"
