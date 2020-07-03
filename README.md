# dsppac program for VUKAP spectrometer

Эта программа предназначена для накопления энергетических и временных
спектров на спектрометре VUKAP version 1.0

## Getting Started

Инструкции в данном документе помогут вам установить программу dsppac.
Протестирована нa Linux Ubuntu 18.04.4 LTS (bionic)

### Prerequisites

What things you need to install.

For the C part:
```
make
g++
libgsl-dev
libusb-1.0-0
libusb-1.0-0-dev
libczmq-dev
cyusb
```

For the Python part:
```
python3
numpy
scipy
matplotlib
pyzmq
gedit
```

### Installing

- Install make, g++, libgsl-dev, libusb, libczmq-dev
```
sudo apt update
sudo apt upgrade
sudo apt install build-essential libgsl-dev libusb-1.0-0 libusb-1.0-0-dev libczmq-dev
```

- Install cyusb
```
sudo apt install git qt5-qmake
git clone https://github.com/Ho-Ro/cyusb_linux.git ~/VUKAP
cd ~/VUKAP/cyusb_linux
sudo mkdir -p /etc/udev/rules.d
make all
sudo make install
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```

A step by step series of examples that tell you how to get a development env running
Say what the step will be

```
Give the example
```
