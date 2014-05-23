oniFixer
========

Question
--------
When using Microsoft Kinect sensor with lastest OpenNI(2.2) and NiTE(2.2), user may have problem using recorded oni file.


This program is because NiTE require a specail property ```XN_STREAM_PROPERTY_PARAM_COEFF``` in depth VideoStream, but it wan't been saved when record ONI file.
This issue make NiTE crash when create UserTracker when using such ONI file.

Fix
--------
The patch had been commit to OpenNI pull request: https://github.com/OpenNI/OpenNI2/pull/80

You can also find pre-compiled patched file here: https://github.com/KHeresy/OpenNI2/releases/tag/KinectOniFix

This Program
--------
For existed ONI files, you can use this program to fix them.

oniFixer use the VirtualDeviceForOpenNI2 project (https://github.com/VIML/VirtualDeviceForOpenNI2), open the exist ONI file and create a virtual device to record it again with all required properties.

Just use command line:

```
oniFixer.exe old.oni new.oni
```

The new.oni should work again!

Notice: oniFixer need to open a physical device to load the required properties.

Download
--------
You can download pre-compiled binary here
https://github.com/VIML/oniFixer/releases

Issue
--------
- No compression of lossy color stream, the file may be very large
- Not process IR stream in this version
