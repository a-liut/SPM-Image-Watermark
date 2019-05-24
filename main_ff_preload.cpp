#include <iostream>
#include <vector>
#include <exception>
#include <dirent.h>
#include <string.h>
#include <chrono>
#include <fstream>

#include <ff/farm.hpp>
#include <ff/pipeline.hpp>

// #define VERBOSE

#include "iwm.cpp"
#include "lib/CImg/CImg.h"

#include "class/blocking_queue.cpp"
#include "class/performance.cpp"
#include "class/job.cpp"

#include "nodes_ff_preload/emitter.cpp"
#include "nodes_ff_preload/stage2.cpp"
#include "nodes_ff_preload/stage3.cpp"
#include "nodes_ff_preload/collector.cpp"

using namespace std;

/**
 * Global stamp image
 */
cimg_library::CImg<CIMG_TYPE> stamp;

/**
 * Check whether a file exists
 */
bool file_exists(const string &path)
{
    ifstream ifile(path);
    return (bool)ifile;
}

/**
 * Global performance collector
 */
iwm::performance perf;

/**
 * Main: validate the input and set up the computation.
 */
int main(int argc, char **argv)
{
    if (argc < 4)
    {
        cout << ": usage: <par_degree> <imgDir> <stampFilename> <delay>" << endl;
        return 0;
    }

    int degree = atoi(argv[1]);
    string imgDir = argv[2];
    string stampFilename = argv[3];
    int delay = atoi(argv[4]);

    if(degree < 1)
    {
        cerr << "invalid parallelism degree: " << degree << endl;
        return 1;
    }

    if(!file_exists(imgDir))
    {
        cerr << "Image directory not found: " << imgDir << endl;
        return 1;
    }

    if(!file_exists(stampFilename))
    {
        cerr << "Stamp not found: " << stampFilename << endl;
        return 1;
    }

    if(delay < 1)
    {
        delay = 0;
    }

#ifdef VERBOSE
    cout << "degree: " << degree << endl;
    cout << "imgDir: " << imgDir << endl;
    cout << "stampFilename: " << stampFilename << endl;
    cout << "delay: " << delay << endl;
#endif

#ifdef VERBOSE
    cout << "Preparing stamp image..." << endl;
#endif

    // Prepare stamp image
    auto stamp_start = perf.now();
    try
    {
        stamp = cimg_library::CImg<CIMG_TYPE>(stampFilename.c_str());
    }
    catch (cimg_library::CImgIOException &ex)
    {
        cerr << "Cannot load stamp image " << stampFilename << "(" << ex.what() << ")" << endl;
        return 1;
    }
    auto stamp_end = perf.now();

#ifdef VERBOSE
    cout << "Done!" << endl;

    cout << "Setting up the pipeline..." << endl;
#endif

    auto setup_start = perf.now();

    // // Setting up the farm
    std::vector<std::unique_ptr<ff_node>> W;
    for(int i = 0; i < degree; ++i)
    {
        W.push_back(make_unique<ff_Pipe<>>(
                        make_unique<Stage2>(&stamp, &perf),
                        make_unique<Stage3>(&perf)
                    ));
    }

    ff_Farm<iwm::Job, iwm::Job> farm(
        move(W),
        unique_ptr<ff_node_t<string, iwm::Job>>(make_unique<Emitter>(imgDir, delay, &perf)),
        unique_ptr<ff_node_t<iwm::Job, iwm::Job>>(make_unique<Collector>(&perf))
    );

    auto setup_end = perf.now();

#ifdef VERBOSE
    cout << "Waiting for termination..." << endl;
#endif

    auto start = perf.now();

    if (farm.run_and_wait_end() < 0)
    {
        cerr << "Error running the pipeline" << endl;
        return 1;
    }

    auto end = perf.now();
    
    perf.setStampTime(stamp_start, stamp_end);
    perf.setSetupTime(setup_start, setup_end);
    perf.setCompletionTime(start, end);

    cout << "---Results---" << endl;

    cout << "Version: ff_preload" << endl;
    cout << "Parallelism Degree: " << degree << endl;
    cout << "Delay: " << delay << endl;
    perf.print();

    cout << "Bye!" << endl;

    return 0;
}