This software was written by Rafael Iriya in Februrary 2017 following paper “Simulation of face/hairstyle swapping in photographs with skin texture synthesis” by Jia-Kai Chou and Chuan-Kai Yang.

OpenCV and compiler versions:
	Code was originally developed using Visual Studio 2017 and visual c++ 14.0 compiler. openCV 2.4.13. The code will not work with openCV version later than 3.0 because of a bug in the cascade detector. 

Dependencies:
	The STASM software Version 4.1.0 is needed to run the code. Older versions will not work as they provide different locations and index of the facial key points. 

Building the code:
	If using compatible Visual Studio IDE, simply open HairSwapping.sln and compile the code.
	If not:
	Include all STASM headers and source files in the project.
	Include all source and header files developed for this project, which can be found in the root project folder	

Running the code:
	The software takes two inputs, model and target, where model refers to the name of the model image (with extension) and target to the name of the target image (with extension). Images need to be inside the data/ folder.	
	If inputs are not properly passed, the software will try to load two example images from the data/ folder.
	The final hair swap image will be shown in screen. The user will need to press a button 	
	After button press, mosaic result image will be saved in the results/ folder


 