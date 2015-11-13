# Distributed-Renderer

### Project Proposal

Work on a renderer that is more than a Cornell box. Aim to render a more realistic scene using 3D models (obj meshes). Distribute it over the network to gain significant speed up.

Start with implementing a Kd­tree to optimize obj rendering. Add more realistic shaders like physically correct subsurface scattering and translucent glass. The program will consists of 2 parts, front­end display and back­end renderer.

* The front end display will be built with OpenGL/C++ and will be responsible for rendering “cheap” version of the scene.

* The back end renderer is a CUDA/C++ path tracer. This is aimed to be run in the background. If possible, we would like to allow the user to set the % of the resources that the program could use, so that when the running computer is available (i.e. the user just uses his powerful machine to surf the internet), other machines could distribute the work to that particular machine. (Basically treat every machine in the ring as a resource pool) Once the user opens the back­end renderer, the user will be prompted to input an IP address of any machine that enables the back­end renderer. (or we could rent a cloud server to just maintain this “active user” list) This will form a “Render Ring” and the program will elect a leader who will be responsible for distributing the scene file and the work load. (and re­elect if that particular machine fails or leave the ring) The user who opens the front­end display can join the ring and send the scene file into the ring. The leader will be responsible to distribute the scene file and information to other machines in the ring, and those machine will render parts of the screen and directly (and continuously, i.e. every x iterations) send those image back to the front end display. The user of the front­end display will be able to move around, print screen, or modify parameters/scene on the fly (to some extent).
