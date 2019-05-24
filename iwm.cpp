#ifndef IWM_LIB
#define IWM_LIB

#define CIMG_TYPE unsigned char
#define OUT_SUFFIX "out_"

#ifndef TIME_UNIT_T
#define TIME_UNIT_T "ms"
#endif

#include "lib/CImg/CImg.h"

using namespace std;

namespace iwm
{

    /**
     * Formats and prints the results of the computation
     */
    void print_results(int parallelismdegree, int setup, int servicetime)
    {
        cout << "---Results---" << endl;

        cout << "Parallelism Degree: " << parallelismdegree << endl;
        cout << "Setup: " << setup << TIME_UNIT_T << endl;
        cout << "Ts = " << servicetime << TIME_UNIT_T << endl;

        cout << "-------------" << endl;
    }

    /**
     * Returns true if the specified pixel of the image is black
     */
    bool is_black(cimg_library::CImg<CIMG_TYPE> &src, int x, int y)
    {
        return src(x, y) == 0;
    }

    /**
     * Returns the gray scale of the specified color
     */
    int gray_scale(int r, int g, int b)
    {
        return (((r + g + b) / 3) + 255) / 2;
    }

    /**
     * Check if the file is valid to be processed
     */
    bool is_valid_file(char *path)
    {
        return strcmp(path, ".") != 0 && strcmp(path, "..") != 0;
    }

    /**
     * Reads the file names in a directory and put them in the vector.
     */
    void read_filenames(const string &imgdir, vector<string *> &filenames)
    {
        DIR *dirPtr = NULL;
        dirPtr = opendir(imgdir.c_str());
        if(dirPtr != NULL)
        {
            // Clear the vector
            filenames.erase(filenames.begin(), filenames.end());

            struct dirent *filePtr = readdir(dirPtr);
            while (filePtr)
            {
                if (iwm::is_valid_file(filePtr->d_name))
                {
                    // Build the real filename
                    string *filepath = new string(imgdir);
                    filepath->append(filePtr->d_name);

                    filenames.push_back(filepath);
                }

                filePtr = readdir(dirPtr);
            }

            closedir(dirPtr);
        } else {
            throw "read_error";
        }
    }

    /**
     * Construct a new output filename
     */
    string get_new_filename(string original)
    {
        int pos = original.length();
        int lastSlashPos = original.find_last_of("/\\");
        return original.insert(lastSlashPos + 1, OUT_SUFFIX);
    }

    /**
     * Prints the stamp on the image inside the specified range
     */
    void print_stamp(cimg_library::CImg<CIMG_TYPE> &image, cimg_library::CImg<CIMG_TYPE> &stamp, int left, int top, int width, int height)
    {
        int endw = left + width;
        int endh = top + height;
        for (int x = left; x < endw; x++)
        {

            for (int y = top; y < endh; y++)
            {
                if (is_black(stamp, x, y))
                {
                    int a = gray_scale(image(x, y, 0, 0), image(x, y, 0, 1), image(x, y, 0, 2));

                    // update the source image
                    image(x, y, 0, 0) = a;
                    image(x, y, 0, 1) = a;
                    image(x, y, 0, 2) = a;
                }
            }
        }
    }
}

#endif