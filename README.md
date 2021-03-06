﻿# Distributed-Renderer

**University of Pennsylvania, CIS 565: GPU Programming and Architecture, FINAL PROJECT**

#### Team : 
 * Dome Pongmongkol : <a href="http://www.rpdome.com">Website</a>
 * Sanchit Garg : <a href="http://www.sanchitgarg.com">Website</a>

<img src="images/DistributedRenderer.png"  height="471.5" width="1031">

<img src="images/DOF.png"  height="500" width="500" align="center">


### Project outline

We have implemented a Distributed Renderer. The idea is divide the final image into parts, render parts on different systems and then accumulate the final image on one system. The rendering method being used on the back end is a CUDA based Multiple Importance Sampling (MIS) path tracer. TCP is used for communication between the viewer and the renderers.

### Use of our project

* Our project can be used to render scenes much faster. We add a new level of parallelization based on distributed systems over the CUDA parallelization at every render node.

* A none CUDA viewer node can initialise the rendering on a CUDA based render node. So you can render even if you don't have a CUDA render node.

* We can compare the rendering performance of different systems. Set them as the leader nodes and do not add any other render node. Then you can compare the render times for the systems.



### Slides

Project Proposal : <a href="Slides/CIS565 Final Project.pdf">LINK</a>

Progress Report 1 : <a href="Slides/Progress report 1 CIS565 Final Project.pdf">LINK</a>

Progress Report 2 : <a href="Slides/Progress report 2 CIS565 Final Project.pdf">LINK</a>

Progress Report 3 : <a href="Slides/Progress report 3 CIS565 Final Project.pdf">LINK</a>

Final Report : <a href="Slides/Final Report CIS565 Final Project.pdf">LINK</a>

Video : <a href="https://youtu.be/TMPTr0WPd7I">LINK</a>



## Networking

##### Modes

Our renderer has 3 modes as follows
* Front End Viewer: This mode will send scene files (.txt/ .obj) to the leader to initialize the render. Please note that this mode runs on any machines that runs OpenGL (no CUDA card requires) To run in this mode, set argument as “f scene_file_dir scene_list”, while scene_file_dir is the path to the directory storing the scene files, and scene_list are the file listing every scene files that needs to be transferred to the leader node. Front-end user can also navigate around the scene using the keyboard (W,A,S,D).

* Renderer: This mode will initialize a renderer that will be in standby until it receives scene files and a command from the leader to render. It will periodically send the pixels it’s responsible for back to the Front End Viewer. To run in this mode, set argument as “r”.

* Leader + Renderer: This mode will initialize both a Leader and a Renderer. The Leader will be responsible for receiving a render job from the Front End Viewer and distribute the work to the renderers. To run in this mode, set argument as “f”.
The user will have to manually type in the IP address of the running Renderer nodes while initializing the program in this mode.

##### How the workload distribution works.
**Please note that this system assumes that the network environment is stable. It will tolerate minor packet loss (as it was built on top of TCP), but if it cannot connect to its receiver after trying for a certain amount of time, it will give up and exit.
**For more information about the packets/commands used. please see msg.proto (Google Protobuf Format)

* Initiate rendering nodes (RN)
* Initiate the leader node (LN), fetch in the IP address and port of the running rendering nodes.
* Initiate the front end viewer (FV), fetch in the IP address and port of the leader node.
* FV contacts LN asking if it’s available, if it is not in the middle of rendering, it will send back an ACK with the code OK, if not, it will also send back an ACK, but with the code NO
* if the FV is given OK, it will proceed and send scene files to LN, if not, it will repeat step 4 again.
after LN receives scene files from FV, it will propagate these scene files to the renderers in its list and mark those as “active”. RN receives scene files and initialize itself.
* LN will go through the active RN list and divide the work equally to all of them. For example, if there are 2 active RNs and 640,000 pixels, each of them will be responsible for rendering 320,000 each. it will also decides in every iterations the RN has to send back data to the FV
* Rendering starts
* Once a RN is done with rendering. It will stop, send a message to tell LN, and stay standby.



## Multiple Importance Sampling

This is a global illumination algorithm where for every ray, we sample both the BxDF and the light. Then using weights, add both the contributions.

For every ray, we intersect it with the scene. At this intersection point, we have to determine the color and the outgoing direction. To determine the color, we need to sample both the light and the surface BRDF. The bounce direction is given by the BRDF.
Once we get both the colors, we add them based on power heuristics. The method is explained in PBRT.



##### Light Importance Sampling

The first step of MIS is sampling based on the light. For this, we select a random point on any one of the light sources. Then we shoot a ray from our intersection point to the light. If the light is unobstructed, them we can calculate the LIS term and its PDF. It is important to note that we need to save the light which was sampled for future reference. This is called light importance sampling as we consider the color only if we can reach the sampled point of our light source.

Image below shows the render of the scene using just LIS.

<img src="images/LIS.png"  height="400" width="400">

As you can see the scene does not have a lot of information and is very dark. We can just get color for the point that can reach the light directly.



##### BRDF importance sampling

In this step, based on the material properties, we sample the bounce direction. For example, for lambertian surfaces, we select a random ray in the cosine weighted hemispherical direction. We try and intersect this ray with the same light that was sampled in LIS. If we can reach that light, then the BRDF sampling would contribute to the final color.

Here is a render using just BIS

<img src="images/BIS.png"  height="400" width="400">



##### MIS = BIS + LIS

The next step is to get the direct illumination effect for the renderer. This is adding the contributions from BIS and LIS steps and using power heuristics based weighting to add the colors. The PDF terms from both the steps is used for this weighting.

Here is a render with the direct illumination step of MIS.

<img src="images/MIS.png"  height="400" width="400">



##### Throughput and Indirect illumination

Now at every bounce depth of the ray, we have to find the contribution of that color calculated to the final pixel color. To do that, we use the concept of throughput. We basically start with 1 and at every step, reduce is by a factor calculated based on the BRDF’s pdf and the MIS color. This acts as the percentage contribution for the bounce depth.

For indirect illumination, we start with a color black and keep adding the contribution of every bounce depth. The bounce color is factored by its throughput.



##### Russian Roulette and Ray Termination

After a trace depth of 2, we check the throughput of every ray with a random float generated between 0 and 1. If the throughput is lower than that, the ray is terminated and the color is written to the image buffer. Other conditions for ray termination are if the ray escapes into the scene and does not intersect anything or a ray hits a light source.

Here is the final image with indirect illumination. 

<img src="images/MIS indirect illumination.png"  height="400" width="400">



## Performance Analysis
The major bottlenecks for this project is the network latency. So we need to find the optimal frequency of sending data (Too often = Slower, but the rendered image on the Front End Viewer is more up-to-date). Taking from Monte Carlo rendering, to get twice a better image, we need to render 4 times more samples. This would be used to decide the frequency of sending data.

A good iterations to update the viewer looked to be 50 iterations. It takes about 42 seconds to do 50 iterations including the data sending to the viewer. So the viewer updates around every 40 seconds.

We compared the performance of our system when we send the image to the viewer at different number of iterations. As expected, when we send the image more frequently, the network latency overshadows the performance. So there is a tradeoff between how frequent we want the viewer image to be refreshed against how much time we want to spend on sending the image. The time of execution is in seconds.

<img src="Analysis/networkLatencyTable.png" height="161" width="362.6">

<img src="Analysis/networkLatency.png" height="588" width="726.6">


Comparing the performance of the system with different number of renderers, we can see a gain with more render nodes. We could test it with only 1 or 2 render nodes and here are the results. The performance increases if we have more render nodes. The time of execution is in seconds.

<img src="Analysis/comparisonNumRenderersTable.png" height="144.2" width="483">

<img src="Analysis/comparisonNumRenderers.png" height="603.4" width="740.6">

We got a near 2x speed up with 2 render nodes as compared to one. The results here show a little more that 2x speed up but that is because of the difference in the render node performances. We used the following systems :

* Mac Book Pro Early 2013, Windows 7, i7 @ 2.4 GHz, GT 650M 1GB
* ASUS ROG GL552VW-DH71, Windows 10, i7 @ 2.6GHz, GTX 960M 2GB

The render time comparison between the 2 systems is as follows :

<img src="Analysis/systemComparisonTable.png" height="105" width="520.8">

<img src="Analysis/systemComaprison.png" height="546" width="728">



## References
* <a href="http://www.pbrt.org/">Physically Based Rendering Techniques</a>
* Adam Mally’s Slides for CIS 450/560 Fall 2015 : <a href="http://www.cis.upenn.edu/~badler/courses/cis560.html">Link to Course Page</a>
* <a href="https://developers.google.com/protocol-buffers/docs/overview?hl=en">Google Protocol Buffer documentation</a>
* <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ms738545(v=vs.85).aspx">Getting Started with Winsock (Windows)</a>



## Google Protocol Buffer
To compile protobuf API, Simply download a Windows binary from its website and set the directory to PATH,
and install google protocol buffer on the machine you wants to compile.

If you want to make any change to the message class, edit the [.proto] file and compile it with

	protoc -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/message.proto

	where 
		$SRC_DIR = the directory containing .proto
		$DST_DIR = destination directory (cpp files dir) 

	ie. protoc -I=C:\Users\Dome\Documents\GitHub\Distributed-Renderer --cpp_out=C:\Users\Dome\Documents\GitHub\Distributed-Renderer\DistRenderer C:\Users\Dome\Documents\GitHub\Distributed-Renderer\msg.proto

To compile the source file, dont forget to put in the [.pb.cc] file and [-lprotobuf] flag
