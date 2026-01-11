# QualiaTouch - a VCV Rack 2 plugin

## About

QualiaTouch is about giving the machine perception and action capabilities with the outside world.

![](doc/qualiatouch-modules.png)

## PhoneSensor module

- PhoneSensor module : retrieves sensor data from the Phyphox app. Not affiliated with PhyPhox, just using their awesome app. Documentation [here](doc/phonesensor.md).

## DepthCamSensor module

- DepthCamSensor module : retrieves hand position and depth from the Kinect sensor from Microsoft. This plugin is not affiliated with Kinect or Microsoft in any way. 🚧 Still experimental 🚧 Documentation [here](doc/depthcam.md).

## DMX modules

If you know about the DMX512 protocol, then you've already understood what these modules do. You'll need an USB -> DMX OUT adapter and an appropriate driver. Documentation [here](doc/dmx.md).

- DMX OUT 1 : allows to send DMX data on one channel from the computer.
- DMX OUT 2 : same with 2 channels.
- DMX OUT 4 : same with 4 channels.

## Contributing

Contributions are much welcome! Especially from C++ experienced people, who'll know how to optimize the execution, clean the code, make it portable and most importantly, prevent memory leaks. Also from svg-friendly people, to improve the module widgets.

## License

As recommended by VCV, QualiaTouch is released under the GNU General Public License v3.0 (or later).

This repository also contains the JSON for Modern C++ library by Niels Lohmann, licensed under the MIT License.

## Thanks

The peole who coded the drivers, the libraries, and the Phyphox app, deserve praise for facilitating the work of many people.
