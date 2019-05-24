#ifndef IWM_JOB
#define IWM_JOB

#include "performance.cpp"

#include "../lib/CImg/CImg.h"

using namespace std;

typedef chrono::time_point<chrono::high_resolution_clock> time_entry;

namespace iwm
{
    /**
     * Represents a task for the farm worker.
     */
    class Job
    {
    private:
        /**
         * Reference to the image to process
         */
        cimg_library::CImg<CIMG_TYPE> *_image;

        /**
         * Reference to the name of the original file
         */
        string *_filename;

        /**
         * Where to store performance results
         */
        perf_entry_t _perf_entry;

    public:
        ~Job()
        {
#ifdef VERBOSE
            cout << "Destroying job..." << endl;
#endif
            delete _image;

            delete _filename;
        }

        void setImage(cimg_library::CImg<CIMG_TYPE> *image)
        {
            _image = image;
        }

        cimg_library::CImg<CIMG_TYPE> *getImage()
        {
            return _image;
        }

        void setFilename(string *filename)
        {
            _filename = filename;
        }

        string *getFilename()
        {
            return _filename;
        }

        perf_entry_t getPerfEntry()
        {
            return _perf_entry;
        }

        void setLatencyStart(time_entry start)
        {
            _perf_entry.latency.first = start;
        }
        void setLatencyEnd(time_entry end)
        {
            _perf_entry.latency.second = end;
        }

        void setTcommEmitterStart(time_entry start)
        {
            _perf_entry.tcomm_emitter.first = start;
        }
        void setTcommEmitterEnd(time_entry end)
        {
            _perf_entry.tcomm_emitter.second = end;
        }
        void setTcommStage1Start(time_entry start)
        {
            _perf_entry.tcomm_stage1.first = start;
        }
        void setTcommStage2Start(time_entry start)
        {
            _perf_entry.tcomm_stage2.first = start;
        }
        void setTcommStage1End(time_entry end)
        {
            _perf_entry.tcomm_stage1.second = end;
        }
        void setTcommStage2End(time_entry end)
        {
            _perf_entry.tcomm_stage2.second = end;
        }

        void setTcommStage3Start(time_entry start)
        {
            _perf_entry.tcomm_stage3.first = start;
        }
        void setTcommStage3End(time_entry end)
        {
            _perf_entry.tcomm_stage3.second = end;
        }
        void setLatencyStage1(time_entry start, time_entry end)
        {
            _perf_entry.latency_stage1.first = start;
            _perf_entry.latency_stage1.second = end;
        }
        void setLatencyStage2(time_entry start, time_entry end)
        {
            _perf_entry.latency_stage2.first = start;
            _perf_entry.latency_stage2.second = end;
        }
        void setLatencyStage3(time_entry start, time_entry end)
        {
            _perf_entry.latency_stage3.first = start;
            _perf_entry.latency_stage3.second = end;
        }
    };
}

#endif