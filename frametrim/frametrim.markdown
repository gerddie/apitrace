# frametrim thought and design notes

## Scope

The program targets at trimming a trace to contain the minimum number
of calls needed to create the image rendered by the target frame.

## Design notes

The programs scans through the trace and collects calls of the various
GL objects and the GL state. At the start of the target frame the
current state is recorded to the output, and all the calls recorded in
the objects used in the callaset corresponding to the frame.

The calls are collected in a std::set so that duplicate calls are
eliminated, and then the set is sorted based on call number and written.


The following types of calls need to be considered:

* State calls: These need to be updated whenever they occure, but only the
  last call before the target frame is drawn needs to be remembered

* Programs: The object must be tracked and calls related to attached
  shaders, uniforms, and attribute arrays

* Textures and buffers: creation and use must be tracked
  - Texture data may also be created by drawing to a fbo
  - Buffer data may be changed by compute shaders

* Frame buffer objects:
  The draw buffer must keep all the state information just like the target
  frame: An attached texture can be used as data source in the target frame
  and an attached renderbuffer can be the source of a blit. In both cases,
  the calls to create the output need to be recorded.
  - Since an FBO needs to keep track of its attachements, and a
    texture/renderbuffer needs to keep track of its data source(s)  a circular
    reference is created, this needs to be handled when writing the calls.

## Tricky things:

* Civilization V
  - A renderbuffer is bound to two different framebuffers, one is used for
    drawing the data, and the other one is used as blit source to copy the
    final data to the screen.
  - It seems the game draws some state display to the default framebuffer
    and then blits the game part to partially fill the defulat framebuffer.
    Problem: The state display is not updated with each frame.

## Next steps:

* code cleanups, move more code out of ft_state.cpp to the respective objects
* Handle all fbo render targtes
* handle fbo blits
* compute shaders and their changeing of buffers and textures (images)
* atomics
* transform feedback and conditional rendering





