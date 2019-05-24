#include <iostream>
#include <vector>
#include <exception>
#include <dirent.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <fstream>

#define EOS NULL

// #define VERBOSE

#include "iwm.cpp"
#include "class/blocking_queue.cpp"
#include "class/performance.cpp"
#include "class/job.cpp"
#include "lib/CImg/CImg.h"

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
 * Send job to the worker i
 */
void send_to_worker(iwm::Job *job, int worker_idx, blocking_queue<iwm::Job *> *workers_queues[])
{
#ifdef VERBOSE
    if(job != NULL) cout << "send_to_worker: " << *job->getFilename() << ", idx: " << worker_idx << endl;
#endif
    workers_queues[worker_idx]->push(job);
}

/**
 * Send the job to all workers
 */
void send_to_all_workers(iwm::Job *job, int nWorkers, blocking_queue<iwm::Job *> *workers_queues[])
{
    for(int i = 0; i < nWorkers; i++)
    {
        send_to_worker(job, i, workers_queues);
    }
}

/**
 * Send the job to workers in a round robin way
 */
int next_worker_idx = -1;
void send_to_worker_rr(iwm::Job *job, int nWorkers, blocking_queue<iwm::Job *> *workers_queues[])
{
    next_worker_idx = (next_worker_idx + 1) % nWorkers;
    send_to_worker(job, next_worker_idx, workers_queues);
}

/**
 * Emitter: Load the image and send it to the workers
 */
void emitter(const string &imgdir, int nWorkers, blocking_queue<iwm::Job *> *workers_queues[], int delay)
{
#ifdef VERBOSE
    cout << "Emitter starts! " << endl;
#endif
    try
    {
        auto start = perf.now();
        vector<string *> filenames;

        iwm::read_filenames(imgdir, filenames);

        vector<iwm::Job *> jobs;
        jobs.reserve(filenames.size());

        // Load all images
        for(string *filepath : filenames)
        {
            cimg_library::CImg<CIMG_TYPE> *image = new cimg_library::CImg<CIMG_TYPE>(filepath->c_str());

            iwm::Job *job = new iwm::Job();
            job->setFilename(filepath);
            job->setImage(image);

            jobs.push_back(job);
        }

        auto stop = perf.now();

        perf.setProcessed(filenames.size());
        perf.setEmitterTime(start, stop);

        bool first = true;
        for(iwm::Job* job : jobs)
        {
            // retard the dispatch of the filename
            if(!first)
            {
#ifdef VERBOSE
                cout << "retard dispatch by " << delay << "ms" << endl;
#endif
                if(delay > 0) std::this_thread::sleep_for (std::chrono::milliseconds(delay));
            }
            else
            {
                first = false;
            }

            auto l_start = perf.now();

            job->setTcommEmitterStart(l_start);

            send_to_worker_rr(job, nWorkers, workers_queues);
        }
    }
    catch(exception &ex)
    {
        cerr << "Error opening image directory " << imgdir << endl;
    }

    // Send EOS to all workers
    send_to_all_workers(EOS, nWorkers, workers_queues);

#ifdef VERBOSE
    cout << "Emitter ends! " << endl;
#endif
}

/**
 * Stage 2: apply the mark
 */
void stage2(blocking_queue<iwm::Job *> *input_queue, blocking_queue<iwm::Job *> *output_queue)
{
#ifdef VERBOSE
    cout << "Stage 2 starts!" << endl;
#endif

    iwm::Job *job = input_queue->pop();
    while(job != EOS)
    {
        auto l_start = perf.now();

        // Apply the transformation
        cimg_library::CImg<CIMG_TYPE> *image = job->getImage();
        iwm::print_stamp(*image, stamp, 0, 0, image->width(), image->height());

        // Send the job to the third stage
#ifdef VERBOSE
        cout << "Worker store the image" << endl;
#endif

        auto l_stop = perf.now();

        job->setLatencyStart(l_start);
        job->setTcommEmitterEnd(l_start);
        job->setLatencyStage2(l_start, l_stop);
        job->setTcommStage2Start(perf.now());

        output_queue->push(job);

        // Take another job
        job = input_queue->pop();
    }

    // Forwarding the EOS
    output_queue->push(job);

#ifdef VERBOSE
    cout << "Stage 2 ends!" << endl;
#endif
}

/**
 * Stage 3: Store the image to the disk
 */
void stage3(blocking_queue<iwm::Job *> *input_queue, blocking_queue<iwm::Job *> *output_queue)
{
#ifdef VERBOSE
    cout << "Stage 3 starts! " << endl;
#endif

    iwm::Job *job = input_queue->pop();
    while(job != EOS)
    {
        // Store the image
#ifdef VERBOSE
        cout << "Worker store the image" << endl;
#endif
        auto l_start = perf.now();

        cimg_library::CImg<CIMG_TYPE> *image = job->getImage();
        string *path = job->getFilename();

        string newfilename = iwm::get_new_filename(*path);

        try
        {
            image->save(newfilename.c_str());
        }
        catch(cimg_library::CImgIOException &ex)
        {
#ifdef VERBOSE
            cerr << "Cannot store the image" << path << endl;
#endif
        }

        auto l_stop = perf.now();
        job->setTcommStage2End(l_start);
        job->setLatencyStage3(l_start, l_stop);

        job->setTcommStage3Start(perf.now());

        output_queue->push(job);

        // Take another job
        job = input_queue->pop();
    }
    // Forwarding the EOS
    output_queue->push(job);

#ifdef VERBOSE
    cout << "Stage 3 ends!" << endl;
#endif
}

/**
 * Collector: collects jobs and update performances results
 */
void collector(int degree, blocking_queue<iwm::Job *> *input_queue)
{
#ifdef VERBOSE
    cout << "Collector starts! " << endl;
#endif
    int remaining_workers = degree;

    iwm::Job *job = NULL;
    while(remaining_workers > 0)
    {
        // Take another job
        job = input_queue->pop();

        if(job != EOS)
        {
            auto end = perf.now();

            job->setLatencyEnd(end);
            job->setTcommStage3End(end);

            perf.registerJob(job);

            delete job;
        }
        else
        {
            // A worker terminates
            remaining_workers--;
        }
    }
#ifdef VERBOSE
    cout << "Collector ends!" << endl;
#endif
}

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
    auto stamp_start = perf.now();

    // Prepare stamp image
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
    // Setting up the farm

    // preparing the pipelines
    blocking_queue<iwm::Job *> **stage2_queue = new blocking_queue<iwm::Job *> *[degree];
    blocking_queue<iwm::Job *> *stage3_queue[degree];
    thread *stage2_workers[degree];
    thread *stage3_workers[degree];

    blocking_queue<iwm::Job *> *collector_queue = new blocking_queue<iwm::Job *>();

    for(int i = 0; i < degree; i++)
    {
        stage2_queue[i] = new blocking_queue<iwm::Job *>();
        stage3_queue[i] = new blocking_queue<iwm::Job *>();
        stage2_workers[i] = new thread(stage2, stage2_queue[i], stage3_queue[i]);
        stage3_workers[i] = new thread(stage3, stage3_queue[i], collector_queue);
    }

    thread th_collector = thread(collector, degree, collector_queue);
    thread th_emitter = thread(emitter, imgDir, degree, stage2_queue, delay);

    auto setup_end = perf.now();

#ifdef VERBOSE
    cout << "Waiting for termination..." << endl;
#endif

#ifdef VERBOSE
    cout << "stopping workers" << endl;
#endif

    auto start = perf.now();

    // Waiting for termination
    th_emitter.join();

    for(int i = 0; i < degree; i++)
    {
        stage2_workers[i]->join();
    }

    for(int i = 0; i < degree; i++)
    {
        stage3_workers[i]->join();
    }

    th_collector.join();

    // Free resources
    for(int i = 0; i < degree; i++)
    {
        delete stage2_queue[i];
        delete stage3_queue[i];

        delete stage2_workers[i];
        delete stage3_workers[i];
    }

    delete[] stage2_queue;
    delete collector_queue;

    auto end = perf.now();

    perf.setStampTime(stamp_start, stamp_end);
    perf.setSetupTime(setup_start, setup_end);
    perf.setCompletionTime(start, end);

    cout << "---Data---" << endl;

    cout << "Version: par_preload" << endl;
    cout << "Parallelism Degree: " << degree << endl;
    cout << "Delay: " << delay << endl;
    perf.print();

    cout << "Done!" << endl;

    return 0;
}