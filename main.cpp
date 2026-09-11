#include <iostream>
#include <fstream>
#include <sstream> 
#include <chrono>
#include <thread>

#include <random>
#include <vector>

#include <math.h>


// Strategy: define the 3 coordinates of a point based on the same (but random) normal distribution so that in a given radius, 
//  every direction is equally possible (uniformly distributed). 
//  Normalize the point on B(0, 1) and multiply it by a random distance following the normal distribution defined by R_mean and R_dev. 
//  Translate the point by (x_c, y_c, z_c).
/* Static function for multithreaded point creation, the core of the program.
    iStart - iEnd:   defining boundaries of vector coordinates to be treated
    (x_c, y_c, z_c): coordinates of centre around which points are created
    (x, y, z):       coordinates of points created
    R_mean, R_dev  : mean value and standard deviation of the distance of the created points from the centre */
void PointCreation(unsigned iStart, unsigned iEnd, double x_c, double y_c, double z_c, 
                   std::vector<double> &x, std::vector<double> &y, std::vector<double> &z, double R_mean, double R_dev)
{
    long seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::normal_distribution<double> normal_coord(0.0, 0.1);
    std::normal_distribution<double> normal_distance(R_mean, R_dev);

    for (unsigned i = iStart; i < iEnd; i++)
    {
        // creating random, uniformly distributed directions from normally distributed coordinates
        x[i] = normal_coord(generator);
        y[i] = normal_coord(generator);
        z[i] = normal_coord(generator);

        double norm = sqrt(x[i]*x[i] + y[i]*y[i] + z[i]*z[i]);

        // getting the normally distributed distance
        double d_c = normal_distance(generator);

        // scaling and translating
        x[i] = x_c + x[i] / norm * d_c;
        y[i] = y_c + y[i] / norm * d_c;
        z[i] = z_c + z[i] / norm * d_c;
    }
}

/* Class that will read input, create points and write them to file on creation of object */
class PointCreator
{
  private:
    int nb_threads;
    unsigned nb_points;
    double x_c, y_c, z_c, R_mean, R_dev;
    std::vector<double> x, y, z;  // coordinates of points to be created

  public:
    PointCreator() = delete;

    PointCreator(std::fstream& inStream, const std::string &outFilename)
    {
        ReadData(inStream);
        CreatePoints();
        WriteResult(outFilename);
    }

  private:
    /* Reads and stores data from input file */
    void ReadData(std::fstream& inStream)
    {
        std::string line;
        getline(inStream, line);
        
        // reading values in line sequentially
        std::istringstream iss(line);
        iss >> nb_threads;
        iss >> nb_points;
        iss >> x_c;
        iss >> y_c;
        iss >> z_c;
        iss >> R_mean;
        iss >> R_dev;
        inStream.close();

        if (fabs(R_mean) < std::numeric_limits<double>::epsilon() && fabs(R_dev) < std::numeric_limits<double>::epsilon())
        {
            std::cout << "Warning: R_mean and R_dev seem both to be zero. ";
            std::cout << "At least one of them should be non-zero to define a proper distribution." << std::endl;
        }
    }


    /* Creates points (through call to PointCreation): times task and takes care that correct portions of coordinate vectors are 
        sent to PointCreation */
    void CreatePoints()
    {
        // starting timer
        auto start = std::chrono::high_resolution_clock::now();

        x.resize(nb_points);
        y.resize(nb_points);
        z.resize(nb_points);

        // multithreading the point creation
        std::vector<std::thread> threads(nb_threads);
        for (int i = 0; i < nb_threads; i++)
        {
            unsigned iStart = i * nb_points / nb_threads;
            // make sure last thread will treat all remaining points and not a (nb_points/nb_threads) slice
            unsigned iEnd = (i != nb_threads - 1) ? (i + 1) * nb_points / nb_threads : nb_points;
            threads[i] = std::thread(PointCreation, iStart, iEnd, x_c, y_c, z_c, std::ref(x), std::ref(y), std::ref(z), R_mean, R_dev);
        }
        for (auto iter = threads.begin(); iter != threads.end(); iter++)
        {
          (*iter).join();
        }

        // ending timer and writing timer result
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::cout << "Time taken to generate points: " << (double)duration.count() / 1000.0 << " milliseconds" << std::endl;
    }

    /* Writes result to output file */
    void WriteResult(const std::string &fileName)
    {
        std::ofstream outputFile(fileName);
        for (int i = 0; i < (int)x.size(); i++)
        {
            outputFile << x[i] << ", " << y[i] << ", " << z[i] << std::endl;
        }
        outputFile.close();
    }
};


int main() 
{
    // data file to be read
    std::string fileName = "./input/input0.txt"; //THIS HERE NEEDS TO CHANGE, INCLUDING THE FILENAME FOR PERFORMANCE BENCHMARKING

    std::fstream inStream;
    inStream.open(fileName, std::ios::in);
    if(inStream.is_open()) 
    {
        std::string outFilename = "./res/res0.csv";
        PointCreator p = PointCreator(inStream, outFilename);
    }
    else
    {
        std::cout << "Could not open input file" << std::endl;
    }
}
