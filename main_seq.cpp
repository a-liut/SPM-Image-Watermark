#include <iostream>
#include <exception>
#include <dirent.h>
#include <string.h>
#include <chrono>
#include <fstream>
#include <vector>

#define CIMG_TYPE unsigned char
#define TIME_UNIT std::chrono::milliseconds
#define TIME_UNIT_T "ms"

// #define VERBOSE

#include "iwm.cpp"
#include "lib/CImg/CImg.h"

using namespace std;

/**
 * Check whether a file exists
 */
bool file_exists(const string &path)
{
    ifstream ifile(path);
    return (bool)ifile;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cout << "usage: <imgDir> <stampFilename>" << endl;
        return 0;
    }

    string imgDir = argv[1];
    string stampFilename = argv[2];

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

#ifdef VERBOSE
    cout << "imgDir: " << imgDir << endl;
    cout << "stampFilename: " << stampFilename << endl;
#endif

    auto stamp_start = chrono::high_resolution_clock::now();
    // Load stamp image
    cimg_library::CImg<CIMG_TYPE> stamp;
    try
    {
        stamp = cimg_library::CImg<CIMG_TYPE>(stampFilename.c_str());
    }
    catch (cimg_library::CImgIOException &e)
    {
        cerr << "Cannot load stamp image " << stampFilename << "(" << e.what() << ")" << endl;
        return 1;
    }

    auto stamp_end = chrono::high_resolution_clock::now();

    auto start_seq = chrono::high_resolution_clock::now();

    vector<string *> filenames;
    try
    {
        iwm::read_filenames(imgDir, filenames);
    }
    catch(exception &ex)
    {
        cerr << "Cannot open image directory " << imgDir << endl;
        return 1;
    }

    auto end_seq = chrono::high_resolution_clock::now();

    auto start = chrono::high_resolution_clock::now();

    for(string *filepath : filenames)
    {
        try
        {
            // Load the image
            cimg_library::CImg<CIMG_TYPE> *image = new cimg_library::CImg<CIMG_TYPE>(filepath->c_str());

            // Apply the stamp
            iwm::print_stamp(*image, stamp, 0, 0, image->width(), image->height());

            // Get the filename for the new file
            string newfile = iwm::get_new_filename(*filepath);

            // Store the new image
            image->save(newfile.c_str());

            delete image;
#ifdef VERBOSE
            cout << "Stored " << newfile << endl;
#endif
        }
        catch (exception &e)
        {
#ifdef VERBOSE
            cerr << "Cannot load image " << filepath << "(" << e.what() << ")" << endl;
#endif
        }
    }

    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> stamp_time = stamp_end - stamp_start;
    chrono::duration<double, milli> sequential_time = end_seq - start_seq;
    chrono::duration<double, milli> completion_time = end - start;

    // Print results
    cout << "stamp time: " << stamp_time.count() << endl;
    cout << "sequential time: " << sequential_time.count() << endl;
    cout << "Tc: " << completion_time.count() << endl;

    cout << "Done!" << endl;
    return 0;
}