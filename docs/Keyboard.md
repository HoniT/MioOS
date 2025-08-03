# Overview of using the keyboard driver

The MioOS keyboard driver uses Key Events and a buffer to save those events.

## Key Event structure

Once a key is pressed the keyboard driver stores the character gotten from the scancode and press state (0- down, 1- up) in a KeyEvent struct object. After that the key event is stored in a buffer using kbrd::push_key_event(KeyEvent ev).

## Retreiving and using key events

For retreiving the key events you can use the kbrd::pop_key_event(KeyEvent& out) in a while loop and use the data as you desire. For an example you can see kterminal_handle_input() in kterminal.cpp (Kernels built in CLI).
