# GdPd

GdPd is a Godot add-on for loading and processing Pure Data patches.
It is meant for fast and flexible prototyping of audiovisual applications 
by allowing one to rapidly choose from and switch between three modes :

* OSC : OpenSoundControl messages are sent between the Godot application and an
  instance of Pure Data running the patch, in order to allow for live edition of
  the sound synthesis and/or low latency audio processing.
* Audio : The audio is played back through Godot sound system, meaning that it
  can be ran on android devices for example.
* Audio RT : The audio is rendered in the application but played using Jack 
    on GNU/Linux and CoreAudio on OSX, for low latency interactions.


GdPd is being developed by the [MINT research team @
CRIStAL](https://mint.univ-lille.fr/) , University of Lille, France.  
For more
details of how it was and can be used, have a look at the paper :
https://hal.science/hal-05095979v1

## Prerequisites

* Pure Data (>=0.54.1) from [https://puredata.info](https://puredata.info) (or
  PurrData, Pd-Lork, plugdata)
* Godot >= 4.3 from [https://godotengine.org/](https://godotengine.org/)

# Using GdPd


1. Install the gdpd folder in the addons folder of your Godot project, for
   example using the asset-lib

2. Create a Pure Data patch that contains at least the *[patches/gdpd]* object, 
    and a number of send/receive objects, for example *[patches/gdpd_receive
    instrumentplaynote]* and *[patches/gdpd_send loudness]*.

3. Add a GdPd node to your scene, either using the 2D or 3D scene editor, or in a 
script : 

```python
var _gdpd : GdPd 

func _ready() :
    # Create the instance and add it to the scene
    _gdpd = GdPd.new()     
    add_child(_gdpd)
    
    # If already in the scene, use for example _gdpd = $GdPd
```

4. Select the integration mode and set the pd patch, either with the inspector 
or in a script :

```python

# Set the mode
_gdpd.set_mode(0) #OSC
_gdpd.set_mode(1) #Audio
_gdpd.set_mode(2) #Audio RT

# Set the main pd patch (can also be done in the inspector)
# for example 
_gdpd.set_pd_patch("patches/patch1.pd")

# Start GdPd
_gdpd.start()
```

5. The interaction with patches is then done as follows :

```python
# Send a message to the Pure Data patch, received with [gdpd_receive from_godot]
# and containing here a list with a string and two integers
_gdpd.send("instrumentplaynote", ["piano", 69, 127])

# Connect to the signal using the inspector, or with the following :
_gdpd.connect("got_message", _on_gd_pd_got_message)

# This method will be called whenever a message is sent from the pd patch with 
# a [gdpd_send] object
func _on_gd_pd_got_message(address : String, arguments : Array) :
    # For example
    if address=="loudness" :
        print("Got loudness with value ", arguments)
```

6. In order to be able to export (e.g. to an apk), add pd patches and other 
    needed files in the export settings (\*.pd, \*.wav) so that they are
    included. Also change the import setting for all audio files in Godot to 
    "Keep File (exported as is)".


For a full working example, open one of the example scenes in the examples
folder.



### Extra features 

TODO 

* Audio input
* Application name
* Network configuration
* Audio device configuration


## Compiling GdPd


### Compiling for GNU/Linux

- Install the following packages :
	- scons
- Open a terminal and type the following commands :

```sh
git clone --recurse-submodules https://gitlab.univ-lille.fr/ivmi/gdpd.git
cd gdpd
scons
```

### Cross-compiling for Windows on GNU/Linux

- Install the following packages :
	- mingw-64
- Type the following commands :

```sh
git clone --recurse-submodules https://gitlab.univ-lille.fr/ivmi/gdpd.git
cd gdpd
scons platform=windows
```

### Compiling for Mac OSX

TODO

### Compiling for Android (/Quest) on GNU/Linux

- Define the ANDROID_HOME environment variable to point to the android ndk
  folder
- Install the following packages :
	- scons
- Open a terminal and type :

```sh
git clone --recurse-submodules https://gitlab.univ-lille.fr/ivmi/gdpd.git
cd gdpd
scons platform=android
```

### Compiling for the Web

TODO

