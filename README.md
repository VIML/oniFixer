oniFixer
========

When using Microsoft Kinect sensor with lastest OpenNI(2.2) and NiTE(2.2), user may have problem using recorded oni file.

This program is because NiTE require a specail property XN_STREAM_PROPERTY_PARAM_COEFF in depth VideoStream, but it wan't been saved when record ONI file.

The patch had been commit to OpenNI pull request: https://github.com/VIML/VirtualDeviceForOpenNI2
You can also find patched file here: https://github.com/KHeresy/OpenNI2/releases/tag/KinectOniFix
