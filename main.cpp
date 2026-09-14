#include <iostream>
#include <fstream>
#include <sstream> 
#include <chrono>
#include <thread>
#include <string>
#include <random>
#include <vector>
#include <map>

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
    std::default_random_engine generator(seed + (long)(10*iStart)); // adding a different offset to the seed for each thread to avoid same random numbers in each thread
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
    long unsigned nb_points;
    double x_c, y_c, z_c, R_mean, R_dev;
    std::vector<double> x, y, z;  // coordinates of points to be created

    std::string number_of_threads = "number of threads",
                number_of_points = "number of points",  
                center_x_coordinate = "center x-coordinate",
                center_y_coordinate = "center y-coordinate",
                center_z_coordinate = "center z-coordinate",
                radius_mean_value = "radius mean value",
                standard_deviation = "standard deviation";

    std::map<std::string, size_t> inputData; // to store input data read from file

  public:
    PointCreator() = delete;

    PointCreator(std::ifstream& inStream, const std::string &outFilename)
    {
        ReadData(inStream);
        CreatePoints();
        WriteResult(outFilename);
    }

  private:
    /* Reads and stores data from input file */
    void ReadData(std::ifstream& inStream)
    {
        inputData = {
            {number_of_threads, false},
            {number_of_points, false},
            {center_x_coordinate, false},
            {center_y_coordinate, false},
            {center_z_coordinate, false},
            {radius_mean_value, false},
            {standard_deviation, false}
        };

        std::string line;
        while (std::getline(inStream, line)) 
        {
            if (line.empty())
                continue; // skip empty lines
            else
                if (line[0] == '#' || line[0] == '\n') 
                    continue; // comment line, skip
                else if (line[0] == ' ')                    
                    throw std::runtime_error("Error: Input file contains a line starting with whitespace. Please check the input file format.");

            size_t pos = line.find_first_of(":");
            if (pos == std::string::npos)
            {
                throw std::runtime_error("Error: Input file contains a line without a colon.\nPlease check the input file format at line '" + line + "'.");
            }
            else
            {
                if (line[pos - 1] == ' ') 
                    throw std::runtime_error("Error: Input file contains a line with a space before the colon.\nPlease check the input file format.");

                std::string key = line.substr(0, pos);
                bool validKey = (inputData.find(key) != inputData.end());
                if (!validKey) 
                    throw std::runtime_error("Error: Unrecognized key in input file: \'" + key + "\'.\nPlease check the input file format.");
            
                pos = pos + (line.substr(pos + 1)).find_first_not_of(" \t"); // +1 to skip ":"
                if (pos == std::string::npos) 
                {
                    throw std::runtime_error("Error: Input file contains a line without a value after the colon for: \'" + key + "\'.\nPlease check the input file format.");
                }
                else if (inputData[key]) // check if key has already been found
                {
                    throw std::runtime_error("Error: Duplicate key in input file for: \'" + key + "\'.\nPlease check the input file format.");
                }
                else
                {
                    inputData[key] = true; // mark key as found
                    line = line.substr(line.find_first_of(":") + 1); // get the value part of the line
                }

                if (key == number_of_threads)
                    nb_threads = std::stoi(line);
                else if (key == number_of_points)
                    nb_points = std::stoul(line);
                else if (key == center_x_coordinate)
                    x_c = std::stod(line);
                else if (key == center_y_coordinate)
                    y_c = std::stod(line);
                else if (key == center_z_coordinate)
                    z_c = std::stod(line);
                else if (key == radius_mean_value)
                    R_mean = std::stod(line);
                else if (key == standard_deviation)
                    R_dev = std::stod(line);
            }
        }

        // check if all inputs are given
        for (const auto& pair : inputData) 
            if (!pair.second)
                throw std::runtime_error("Error: Missing input for key: \'" + pair.first + "\'.\nPlease check the input file format.");

        if (fabs(R_mean) < std::numeric_limits<double>::epsilon() && fabs(R_dev) < std::numeric_limits<double>::epsilon())
            throw std::runtime_error("Error: R_mean and R_dev seem both to be zero. \nAt least one of them should be non-zero to define a proper distribution.");
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
            long unsigned iStart = i * nb_points / nb_threads;
            // make sure last thread will treat all remaining points and not a (nb_points/nb_threads) slice
            long unsigned iEnd = (i != nb_threads - 1) ? (i + 1) * nb_points / nb_threads : nb_points;
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
        outputFile << "# Simulation ran with the following parameters:" << std::endl;
        outputFile << "#     " << number_of_threads << ": " << nb_threads << std::endl;
        outputFile << "#     " << number_of_points << ": " << nb_points << std::endl;
        outputFile << "#     " << center_x_coordinate << ": " << x_c << std::endl;
        outputFile << "#     " << center_y_coordinate << ": " << y_c << std::endl;
        outputFile << "#     " << center_z_coordinate << ": " << z_c << std::endl;
        outputFile << "#     " << radius_mean_value << ": " << R_mean << std::endl;
        outputFile << "#     " << standard_deviation << ": " << R_dev << std::endl;
        outputFile << "# ---------------------------------------------------------" << std::endl;
        outputFile << "# Resulting points (x, y, z) in CSV format:" << std::endl;
        outputFile << "#     x, y, z" << std::endl << std::endl;

        for (int i = 0; i < (int)x.size(); i++)
        {
            outputFile << x[i] << ", " << y[i] << ", " << z[i] << std::endl;
        }
        outputFile.close();
    }
};


int main(int argc, char* argv[]) 
{

    std::string filenames = (argc > 1) ? argv[1] : "default";
    std::string inputFileName = "./input/" + filenames + ".txt";
    
    std::ifstream inStream(inputFileName);
    if(!inStream.fail()) 
    {
        std::string outFilename = "./res/" + filenames + ".csv";
        try
        {  
            PointCreator p = PointCreator(inStream, outFilename);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            return 1;
        }
    }
    else
    {
        std::cout << "Could not open input file" << std::endl;
    }
}
