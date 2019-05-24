#ifndef IWM_PERF
#define IWM_PERF

#include <chrono>
#include <iomanip>

using namespace std;

typedef milli ms;
typedef chrono::high_resolution_clock my_time;
typedef chrono::duration<double, ms> fsec;

typedef chrono::time_point<my_time> time_entry;

struct perf_entry_t
{
    pair<time_entry, time_entry> latency;

    pair<time_entry, time_entry> tcomm_emitter;
    pair<time_entry, time_entry> tcomm_stage1;
    pair<time_entry, time_entry> tcomm_stage2;
    pair<time_entry, time_entry> tcomm_stage3;

    pair<time_entry, time_entry> latency_stage1;
    pair<time_entry, time_entry> latency_stage2;
    pair<time_entry, time_entry> latency_stage3;
};

#include "job.cpp"

namespace iwm
{
    class performance
    {
    private:
        int _processed = 0;

        pair<time_entry, time_entry> _tc;
        pair<time_entry, time_entry> _setup;
        pair<time_entry, time_entry> _stamp;

        pair<time_entry, time_entry> _emitter_time;
        vector<perf_entry_t> _entries;
        vector<time_entry> _ts;

        double toMillis(fsec t)
        {
            return t.count();
        }
    public:

        void setProcessed(int p)
        {
            _processed = p;
            _entries.reserve(p);
        }

        void setStampTime(time_entry start, time_entry end)
        {
            _stamp.first = start;
            _stamp.second = end;
        }

        void setSetupTime(time_entry start, time_entry end)
        {
            _setup.first = start;
            _setup.second = end;
        }

        void setCompletionTime(time_entry start, time_entry end)
        {
            _tc.first = start;
            _tc.second = end;
        }

        void setEmitterTime(time_entry start, time_entry end)
        {
            _emitter_time.first = start;
            _emitter_time.second = end;
        }

        void registerJob(iwm::Job *job)
        {
            _ts.push_back(now());
            _entries.push_back(job->getPerfEntry());
        }

        time_entry now()
        {
            return chrono::high_resolution_clock::now();
        }

        void print()
        {
            cout << "---Results---" << endl;
            cout << "Processed: " << _processed << endl;

            fsec stamp_diff = _stamp.second - _stamp.first;
            cout << "Stamp loading: " << toMillis(stamp_diff) << endl;

            fsec setup_diff = _setup.second - _setup.first;
            cout << "Setup: " << toMillis(setup_diff) << endl;

            fsec tc_diff = _tc.second - _tc.first;
            cout << "Tc: " << toMillis(tc_diff) << endl;

            double avg = 0;
            cout << "---Ts---" << endl;
            for(vector<time_entry>::iterator it = _ts.begin(); it != _ts.end() - 1; it++)
            {
                fsec ts_diff = *(it + 1) - *it;
                avg = avg + toMillis(ts_diff);
            }

            avg = avg / (_ts.size());
            cout << "Ts avg: " << avg << endl;

            double lat_avg = 0;
            for(auto &pair : _entries)
            {
                fsec lat_diff = pair.latency.second - pair.latency.first;
                lat_avg = lat_avg + toMillis(lat_diff);
            }
            lat_avg = lat_avg / _entries.size();
            cout << "L avg: " << lat_avg << endl;

            fsec emitter_diff = _emitter_time.second - _emitter_time.first;
            cout << "Emitter: " << toMillis(emitter_diff) << endl;

            cout << "Entries:" << endl;
            cout << std::setw(5) << "n" << std::setw(5) << "|"
                 << std::setw(5) << "L" << std::setw(5) << "|"
                 << std::setw(5) << "L S1" << std::setw(5) << "|"
                 << std::setw(5) << "L S2" << std::setw(5) << "|"
                 << std::setw(5) << "L S3" << std::setw(5) << "|"
                 << std::setw(5) << "Tcom emit." << std::setw(5) << "|"
                 << std::setw(5) << "Tcom 1" << std::setw(5) << "|"
                 << std::setw(5) << "Tcom 2" << std::setw(5) << "|"
                 << std::setw(5) << "Tcom 3" << std::setw(5) << endl;
            cout << "------------------------------------------------------------------------" << endl;
            int idx = 0;
            for(auto &pair : _entries)
            {
                fsec lat_diff = pair.latency.second - pair.latency.first;
                fsec l1_diff = pair.latency_stage1.second - pair.latency_stage1.first;
                fsec l2_diff = pair.latency_stage2.second - pair.latency_stage2.first;
                fsec l3_diff = pair.latency_stage3.second - pair.latency_stage3.first;

                fsec tcomemitter_diff = pair.tcomm_emitter.second - pair.tcomm_emitter.first;
                fsec tcom1_diff = pair.tcomm_stage1.second - pair.tcomm_stage1.first;
                fsec tcom2_diff = pair.tcomm_stage2.second - pair.tcomm_stage2.first;
                fsec tcom3_diff = pair.tcomm_stage3.second - pair.tcomm_stage3.first;

                cout << std::setw(5) << ++idx << std::setw(5) << " "
                     << std::setw(5) << toMillis(lat_diff) << std::setw(5) << " "
                     << std::setw(5) << toMillis(l1_diff) << std::setw(5) << " "
                     << std::setw(5) << toMillis(l2_diff) << std::setw(5) << " "
                     << std::setw(5) << toMillis(l3_diff) << std::setw(5) << " "
                     << std::setw(5) << toMillis(tcomemitter_diff) << std::setw(5) << " "
                     << std::setw(5) << toMillis(tcom1_diff) << std::setw(5) << " "
                     << std::setw(5) << toMillis(tcom2_diff) << std::setw(5) << " "
                     << std::setw(5) << toMillis(tcom3_diff) << std::setw(5)
                     << endl;
            }
        }
    };
}

#endif