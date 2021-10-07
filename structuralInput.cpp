// ConsoleApplication3.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <CL/cl.hpp>
#include <stdio.h>
#pragma comment(lib, "OpenCL.lib")
#define MAX_SOURCE_SIZE (0x100000)

static size_t fileToString(const char *filename, char**fileBuffer) // no file return 0, pointer to pointer means it needs an address of a pointer
{
	size_t fileBufferSize;
	FILE *f;
	f = fopen(filename,"r");
	if(f)
	{
		fseek(f,0,SEEK_END); // After this, the file pointer is at the end.
		long len = ftell(f);
		fseek(f, 0, SEEK_SET); // Re-set the pointer in the beginning of the file.
		*fileBuffer = (char*)malloc(len);
		if(*fileBuffer)
		{
			fileBufferSize = fread(*fileBuffer, 1, len, f);
		}
		fclose(f);
		return fileBufferSize;
	}
	return 0;
}

typedef struct tag_fileBlock
{
	cl_int len;
	char f[50]; //should be cl_char!
}fileBlock;

typedef struct tag_outBuff
{
	cl_int len;
	cl_uchar buff[512];
}outBuff;

int _tmain(int argc, _TCHAR* argv[])
{

	cl_int status =0;

	//choose platform_id from platforms 
	cl_uint numPlatforms;
	cl_platform_id platform =NULL;

	status  = clGetPlatformIDs(0,NULL,&numPlatforms);
	//check if status is CL_SUCCESS

	if(status ==CL_SUCCESS && numPlatforms>0)
	{
		status = clGetPlatformIDs(numPlatforms,&platform,NULL);
		
	}else {exit(0);}

	//choose device(s) from devices
	cl_uint  numDevices;
	cl_device_id  device =NULL;
	status = clGetDeviceIDs(platform,CL_DEVICE_TYPE_ALL, 0,NULL, &numDevices);

	if(status ==CL_SUCCESS && numDevices >0)
	{
		status = clGetDeviceIDs(platform,CL_DEVICE_TYPE_GPU,numDevices, &device, NULL);
	}else {exit(0);}

	//create context & command queue
	cl_context context = clCreateContext(NULL,numDevices,&device, NULL,NULL,NULL);
	cl_command_queue commandQueue = clCreateCommandQueue(context, device, 0,&status);

	//read kernel and create Program
	char filename[] = "./computeSHA3.cl";
	char *source;
	size_t sourceSize = fileToString(filename,&source);

	cl_program prog = clCreateProgramWithSource(context, 1,(const char**)&source, &sourceSize, &status);
	if(status == CL_SUCCESS)
	{
		//sprintf((char array)buildOption, "-g -D WGSIZE=%d", GroupSize);
		status = clBuildProgram(prog,1, &device, NULL,NULL,NULL); // buildOption is put at position 2 [func(0,1,2,3,4)] if there is
	}else{exit(0);}


	if(status !=CL_SUCCESS)  // test the program building
	{
		exit(0);
	}
	// create input buffer and output buffer
	fileBlock input[5];
	input[0].len=11;
	strcpy_s(input[0].f,"hello world");
	//memcpy_s(input[0].f,5, "Hello", 5);
	input[1].len=10;
	strcpy_s(input[1].f,"hello worl");
	input[2].len=9;
	strcpy_s(input[2].f,"hello wor");
	input[3].len=8;
	strcpy_s(input[3].f,"hello wo");
	input[4].len=7;
	strcpy_s(input[4].f,"Hello w");

	fileBlock *p =input;
	size_t inputSize = 5*sizeof(fileBlock);
	
	outBuff out[5];
	outBuff *output =out;
	size_t outputSize = 5*sizeof(outBuff);

	cl_mem inputBuffer = clCreateBuffer(context,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, inputSize, (void*) p, &status);
	cl_mem outputBuffer =clCreateBuffer(context, CL_MEM_WRITE_ONLY,outputSize,NULL,&status);

	//create kernel and set parameters for kernel
	cl_kernel kernel = clCreateKernel(prog, "compute", &status);
	status = clSetKernelArg(kernel,0, sizeof(cl_mem), &inputBuffer);
	status = clSetKernelArg(kernel,1, sizeof(cl_mem), &outputBuffer);



	//en-command queue
	size_t globalSize[1] ={5};// self-defined value
	size_t localSize[1] = {64};//same
	status = clEnqueueNDRangeKernel(commandQueue,kernel,1, NULL,globalSize,NULL, 0,NULL,NULL);

	//extract the output to outputBuffer
	status = clEnqueueReadBuffer(commandQueue,outputBuffer, CL_TRUE, 0, outputSize, output, 0, NULL,NULL); 

	//output[99]='\0';
	//puts(output);

	for(int i=0;i<5;i++)
	{
		printf("Element %d :\n",i);
		printf("len: %d \n", (output+i)->len);
		//printf("buff: %s \n", (output+i)->buff);
		cl_uchar* b =  (output+i)->buff;
		for(int c= 0;c<64;c++)
		{
			printf("%02x", *(b+c));
		}
		printf("\n");
	}

	//wait for the finishing of task queue;
	status = clFinish(commandQueue);

	//close all the related objects
	status = clReleaseKernel(kernel);
	status =clReleaseMemObject(inputBuffer);
	status = clReleaseMemObject(outputBuffer);
	status = clReleaseProgram(prog);
	status =clReleaseCommandQueue(commandQueue);
	status =clReleaseContext(context);

	free(source);
	printf("the status is %d\n",status);
	//printf("%s\n",(p)->f);

	return 0;
}

