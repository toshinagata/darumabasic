CIRCLEHOME=$PWD/../../circle
cd $CIRCLEHOME

if [ -e include/stdint.h ]; then
  mv -f include/stdint.h include/_stdint.h
fi

if [ -e Config.mk ]; then
  mv -f Config.mk Config.mk.save
fi

cat << END_PI1 >Config.mk
RASPPI = 1
ARCH = -march=armv6j -mtune=arm1176jzf-s -mfloat-abi=softfp
END_PI1
./makeall clean
./makeall
rm -rf pi1
mkdir pi1
cp -f lib/startup.o lib/libcircle.a lib/usb/libusb.a lib/input/libinput.a lib/fs/libfs.a lib/fs/fat/libfatfs.a lib/sched/libsched.a lib/net/libnet.a lib/bt/libbluetooth.a pi1

cat << END_PI2 >Config.mk
RASPPI = 2
ARCH = -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=softfp
END_PI2
./makeall clean
./makeall
rm -rf pi2
mkdir pi2
cp -f lib/startup.o lib/libcircle.a lib/usb/libusb.a lib/input/libinput.a lib/fs/libfs.a lib/fs/fat/libfatfs.a lib/sched/libsched.a lib/net/libnet.a lib/bt/libbluetooth.a pi2

if [ -e Config.mk.save ]; then
  mv -f Config.mk.save Config.mk
fi

