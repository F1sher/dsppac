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
sudo cp ./include/cyusb.h /usr/local/include/
sudo cp ./lib/libcyusb.so.1 /usr/local/lib/
sudo cp ./lib/libcyusb.so /usr/local/lib/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```

- Install sclog4c
```
sudo mkdir /usr/local/lib/sclog4c/
sudo mkdir /usr/local/include/sclog4c/
cp ~/VUKAP/dsppac/build/sclog4c/src
make clean
make
sudo cp libsclog4c.a /usr/local/lib/sclog4c/
sudo cp ../include/sclog4c.h /usr/local/include/sclog4c/
```

- Compile hdrainer
```
git clone https://github.com/F1sher/dsppac.git ~/VUKAP
cd ~/VUKAP/build

copy cyusb.h to include
LIBS ./lib/ ??? in Makefile (should be /usr/local/lib or nothing?)
change in code:
cyusb_handle -> libusb_handle
cyusb_kernel_driver_active -> libusb_kernel_driver_active
cyusb_claim_interface -> libusb_claim_interface
cyusb_clear_halt -> libusb_clear_halt
cyusb_bulk_transfer -> libusb_bulk_transfer
cyusb_control_transfer -> libusb_control_transfer
paste "#define JSMN_HEADER" in send_comm.c, hdrainer.c before #include "parse_args.h"

make sclog4c in Makefile? -> install sclog4c to /usr and now in dsp.h #include <sclog4c/sclog4c.h>
make hdrainer
```
