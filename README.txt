oniFixer
========

When using Microsoft Kinect sensor with lastest OpenNI(2.2) and NiTE(2.2), user may have problem using recorded oni file.

This program is because NiTE require a specail property 'XN_STREAM_PROPERTY_PARAM_COEFF' in depth VideoStream, but it wan't been saved when record ONI file.
This issue make NiTE crash when create UserTracker when using such ONI file.

The patch had been commit to OpenNI pull request: https://github.com/VIML/VirtualDeviceForOpenNI2
You can also find patched file here: https://github.com/KHeresy/OpenNI2/releases/tag/KinectOniFix

For existed ONI files, you can use this program to fix them.
oniFixer use the VirtualDeviceForOpenNI2 project (https://github.com/VIML/VirtualDeviceForOpenNI2), open the exist ONI file and create a virtual device to record it again with all required properties.

Just use command line:

oniFixer.exe old.oni new.oni

The new.oni should work again!
