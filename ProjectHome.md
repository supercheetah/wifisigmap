# Simple Wireless Signal Strength Mapper #

Allows you to import a map of your building, then walk around and measure the strength at points on the map. An overlay showing the strength of the signal is rendered on top of your map.

The primary target of this app is/will be Android-powered devices, through use of the Necessitas project. This app is written using the Qt library. Since it's written in Qt, it will also run with no changes whatsoever on Linux - so you could do the mapping on a laptop with Linux and a working WiFi card as well.

# Updates #

## 2012-07-18 - [r81](https://code.google.com/p/wifisigmap/source/detail?r=81) ##

![![](https://lh4.googleusercontent.com/-7Im-5AhS2tA/UAjS2744okI/AAAAAAAABTI/1jsG0huXcfE/s854/WifiMapperApLocatorExample.png)](https://lh4.googleusercontent.com/-7Im-5AhS2tA/UAjS2744okI/AAAAAAAABTI/1jsG0huXcfE/s854/WifiMapperApLocatorExample.png)

Application is feature-complete (at least, all the basic features.) Tested and works on both Linux and Android.

Screenshot, above, shows the "AP Locator" feature which attempts to guess the location of the physical access point based on several (2 or more) signal readings. In the above screenshot, the "BryanHQ" AP is accurately located to within 6 fee. (It's actual location is at the center of the signal reading above and to the right of it's calculate location - the scale of the map is 15.25px = 1 foot.)