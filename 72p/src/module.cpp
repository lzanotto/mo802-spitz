/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Caian Benedicto <caian@ggaunicamp.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */


// This define enables the C++ wrapping code around the C API, it must be used
// only once per C++ module.
#define SPITZ_ENTRY_POINT

// Spitz serial debug enables a Spitz-compliant main function to allow
// the module to run as an executable for testing purposes.
// #define SPITZ_SERIAL_DEBUG

#include <spitz/spitz.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <new>
#include "RayTracer.h"
#include "Image.h"
// This class creates tasks.
using namespace std;
class job_manager : public spitz::job_manager
{

private:
    int i;
public:
    int width;
    int p;
    job_manager(int argc, const char *argv[], spitz::istream& jobinfo) :
        i(0)
    {
        cout << "[JM] Job manager created." << endl;
        width = atoi(argv[5]);
        p = width; //atoi(argv[1]);

    }

    bool next_task(const spitz::pusher& task)
    {
        spitz::ostream o;

        // Serialize the task into a binary stream
        // ...
        if (i >= p)
            return false;

        // Push the current step
        o << i;

        //Increment the step by the amount to be packed for each worker
        i += 1;

        cout << "[JM] Task generated for i = ." << i << endl;

        // Send the task to the Spitz runtime
        task.push(o);

        // Return true until finished creating tasks

        cout << "[JM] The module will run forever until "
            "you add a return false!" << endl;

        return true;
    }

    ~job_manager()
    {
    }
};

// This class executes tasks, preferably without modifying its internal state
// because this can lead to a break of idempotence between tasks. The 'run'
// method will not impose a 'const' behavior to allow libraries that rely
// on changing its internal state, for instance, OpenCL (see clpi example).
class worker : public spitz::worker
{
public:
    int maxReflections;
    int superSamples;
    int depthComplexity;
    int width;
    int height;
    int y;
    RayTracer *rayTracer;
    Color *color_vector;
    worker(int argc, const char *argv[])
   {
           if (argc < 6) {
             cerr << "Usage: " << argv[0] << " sceneFile superSamples " <<
              "depthComplexity [outFile]" << " width " << "height" << endl;
             exit(EXIT_FAILURE);
           }

      srand((unsigned)time(0));
      maxReflections = 10;
      superSamples = atoi(argv[2]);
      depthComplexity = atoi(argv[3]);


      width = atoi(argv[5]);
      height = atoi(argv[6]);


      rayTracer = new RayTracer(width, height, maxReflections, superSamples, depthComplexity);

     if (strcmp(argv[1], "-") == 0) {
         rayTracer->readScene(std::cin);
      } else {
         const char* inFile = argv[1];
         ifstream inFileStream;
         inFileStream.open(inFile, ifstream::in);

         if (inFileStream.fail()) {
            cerr << "Failed opening file" << endl;
            exit(EXIT_FAILURE);
         }

         rayTracer->readScene(inFileStream);
         inFileStream.close();
      }

      int columnsCompleted = 0;
      rayTracer->camera.calculateWUV();



      // Reset depthComplexity to avoid unnecessary loops.
      if (rayTracer->dispersion < 0) {
         rayTracer->depthComplexity = 1;
      }

      rayTracer->imageScale = rayTracer->camera.screenWidth / (float)width;



       cout << "[WK] Worker created." << endl;
   }

    int run(spitz::istream& task, const spitz::pusher& result)
    {
        // Binary stream used to store the output
        spitz::ostream o;

        // Read the current step from the task
        int x, batch;
        task >> x;
        batch = x;
        o << x;

        // Get the number of steps to compute

        for ( ; x < width; x++) {
            color_vector = new Color[height];
            //#pragma omp parallel for
            for (y = 0; y < height; y++){
              color_vector[y] = rayTracer->traceRays(x, y);             
            }
            // Add the term to the data stream
            o.write_data(color_vector, height * sizeof(Color));
        }


        // Deserialize the task, process it and serialize the result
        // ...

        // Send the result to the Spitz runtime
        result.push(o);

        cout << "[WK] Task processed." << endl;

        return 0;
    }
};

// This class is responsible for merging the result of each individual task
// and, if necessary, to produce the final result after all of the task
// results have been received.
class committer : public spitz::committer
{
public:
    int width;
    int height;
    int x;
    string outFile;
    Image *image;
    Color *color_vector;
    committer(int argc, const char *argv[], spitz::istream& jobinfo)
    {
        if (argc > 6) {
          outFile = argv[4];
        } else {
          std::cerr << "No outFile specified - writing to out.tga" << std::endl;
          outFile = "out.tga";
        }

       width =  atoi(argv[5]);
       height = atoi(argv[6]);

       image = new Image(width,height);
       color_vector = new Color[height];
    }

    int commit_task(spitz::istream& result)
    {
        // Deserialize the result from the task and use it
        // to compose the final result
        // ...
        Color color;

        cout << "[CO] Committer created." << endl;
        cout << "\rDone!" << endl;   
        result >> x;
        result.read_data(color_vector, height * sizeof(Color));


        for (int y = 0; y < height; y++){
                  image->pixel(x,y,color_vector[y]);
        }
        cout << "[CO] Result committed." << endl;

        return 0;
    }

    // Optional. If the result depends on receiving all of the task
    // results, or if the final result must be serialized to the
    // Spitz Main, then an additional Commit Job is called.

    int commit_job(const spitz::pusher& final_result)
    {
        // Process the final result

        // A result must be pushed even if the final
        // result is not passed on
        image->WriteTga(outFile.c_str(), false);
        final_result.push(NULL, 0);
        return 0;
    }

    ~committer()
    {

    }
};

// The factory binds the user code with the Spitz C++ wrapper code.
class factory : public spitz::factory
{
public:
    spitz::job_manager *create_job_manager(int argc, const char *argv[],
        spitz::istream& jobinfo)
    {
        return new job_manager(argc, argv, jobinfo);
    }

    spitz::worker *create_worker(int argc, const char *argv[])
    {
        return new worker(argc, argv);
    }

    spitz::committer *create_committer(int argc, const char *argv[],
        spitz::istream& jobinfo)
    {
        return new committer(argc, argv, jobinfo);
    }
};

// Creates a factory class.
spitz::factory *spitz_factory = new factory();
