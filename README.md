Build based on https://github.com/friction07/corne-xiao whith a dongle.

On the dongle:
- nice!view
- custom colors on the Xiao's status led to display the current active layer

On the left side:
- nothing

On the right side:
- Encoder

The keymap is designed to-
- write in both English and French
- have some sort of keypad (I hate numbers on a row and I want to be able to type them all with the left hand only)
- have macro dedicated to writing À, É, È, Ù, etc. with US international keyboard in Windows. It works pretty okish on Mac, but hasnt been tested thoroughly.

That is also my first build on ZMK. My learning so far:
- dont forget to pair the halves together before anything, flashing is not enough. It seems so obvious today, but I had to reach for help to sort this one out.
- the nice!view requires some pins to be defined, and the CS one is not declared in the same place as the others
- there are cases when the config lines in the config files of the board\shields\whatever folder are not taken into account for some modules. If this happens, placing the config items in the build.yaml in a cmake-args allows the override
- there is a "C" in "cmake-args", and config "ZMK-config" in a config file becomes "DZMK-config" with a "D" in cmake-args line
  

