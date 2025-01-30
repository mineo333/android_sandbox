For this to work, push the apk to `/data/local/tmp/[yourapk].apk` and replace the path in main.cpp

The purpose of this is to be able to load a JNI shared object and then poke around it. The resulting executable is able to create a [`Context`](https://developer.android.com/reference/android/content/Context) and do most things that an Android app can do. 

The objective here is to be able to create an environment in which one can poke around an android app without the need for Frida

Based on: 

https://github.com/quarkslab/android-fuzzing/tree/main

https://github.com/rednaga/native-shim/tree/master

https://github.com/frida/frida-java-bridge
