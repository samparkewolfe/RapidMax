#RapidMax

This is the rapidlib c++ machine learning library wrapped into a max object.
(http://doc.gold.ac.uk/eavi/rapidmix/docs_cpp/annotated.html)

This repo includes the max 7 sdk as well as the source code for the external.

Build the external though the Xcode project found in source/myexternals/rapidmax/rapidmax.xcodeproj and the .mxo object will appear in the externals folder.

See rapidmax.maxhelp for how to use the object, just drag the rapidmax.mxo into the help folder and reopen max for the helpfile to find it.



# max-sdk

The Max Software Development Kit contains the API (headers, source, libraries) for building external objects (plug-ins) in C/C++ for [Max](https://cycling74.com/max7/). It additionally includes documentation of the API and example projects using the Xcode 6 and Visual Studio 2013 development environments.

The SDK is structured as a Max package. Simply place the SDK in your `Max 7/Packages` folder to begin working.

## testing

It is highly recommended that you test your code thoroughly. One option is use the [max-test](https://github.com/Cycling74/max-test) package.

## support
 
For support, please use the developer forums at:
http://cycling74.com/forums/