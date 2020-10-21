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

* sauerbraten doesn't start all frames with a clear, so it is not clear whether
  one can throw away old ftaw calls
* in the start screen sauerbraten only updates part of the screen, so one
  must keep all the calls.
* Blender 2.79 copies from the default framebuffer (backbuffer), so trimming
  this is likely hopeless.

## Currently known problems with v2

* In the HL2 startup all text is missing

* in CIV5 the terrain tiles are not retained and some icons are not drawn
  correctly, this is probably a texture attachement problem. One can work
  around this by selecting a series of frames. The current implementation
  will put all calls of a continuous series of frames in to the setup frame
  extept the calls from the last frame of such a series, this will go
  into an extra frame

* Sauerbraten has some incorrect geometry

* Portal2 has some geometry missing, but this can also be fixed like Civ5
  by specifying a range of frames including the setup frame that can be
  identified by using qapitrace. Setup frames usually ha a significant larger
  number of calls like other frames.

* GolfWithYourFriends uses GL_COPY_WRITE_BUFFER, but apparently only to
  map the buffer and write data to it, and not for its actual purpouse
  (glCopyBufferSubData)

## Notes for optimization

* when mapping with GL_MAP_INVALIDATE_BUFFER_BIT we can drop earlier mapping
  or subdata calls

## TODO

* glBindBufferRange
* glCopyTexSubImage2D

