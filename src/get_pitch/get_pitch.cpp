/// @file

#include <iostream>
#include <fstream>
#include <string.h>
#include <errno.h>

#include "wavfile_mono.h"
#include "pitch_analyzer.h"

#include "docopt.h"

#define FRAME_LEN   0.030 /* 30 ms. */
#define FRAME_SHIFT 0.015 /* 15 ms. */

using namespace std;
using namespace upc;

static const char USAGE[] = R"(
get_pitch - Pitch Estimator 

Usage:
    get_pitch [options] <input-wav> <output-txt>
    get_pitch (-h | --help)
    get_pitch --version

Options:
    -h, --help  Show this screen
    --version   Show the version of the project
    -a REAL, --u_pot=REAL  Umbral de la potencia [DEFAULT: 20]
    -b REAL, --u_r1=REAL  Umbral r[1]/r[0] [DEFAULT: 0.8]
    -c REAL, --u_rmax=REAL  Umbral r[lag]/r[0] [DEFAULT: 0.9]
    --preprocess <type> Preprocessing type [default: none].
                        Type can be:
                        - 'clip': central clipping
                        - 'lowpass': low-pass filter
    --postprocess <type> Postprocessing type [default: none].
                        Type can be:
                        - 'median': median filter.
                        - 'timewarp': time-warping.
    --median <size>     Size of the median filter [default: 1].
    
Arguments:
    input-wav   Wave file with the audio signal
    output-txt  Output file: ASCII file with the result of the estimation:
                    - One line per frame with the estimated f0
                    - If considered unvoiced, f0 must be set to f0 = 0
)";

int main(int argc, const char *argv[]) {
	/// \TODO 
	///  Modify the program syntax and the call to **docopt()** in order to
	///  add options and arguments to the program.
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        {argv + 1, argv + argc},	// array of arguments, without the program name
        true,    // show help if requested
        "2.0");  // version string

	std::string input_wav = args["<input-wav>"].asString();
	std::string output_txt = args["<output-txt>"].asString();
  float u_r1 = stof(args["--u_r1"].asString());
  float u_rmax = stof(args["--u_rmax"].asString());
  

  /*std::string preprocess_type = args["--preprocess"].asString();
  std::string postprocess_type = args["--postprocess"].asString();*/
  unsigned int median_size = stof(args["--median"].asString());

  // Read input sound file
  unsigned int rate;
  vector<float> x;
  if (readwav_mono(input_wav, rate, x) != 0) {
    cerr << "Error reading input file " << input_wav << " (" << strerror(errno) << ")\n";
    return -2;
  }

  int n_len = rate * FRAME_LEN;
  int n_shift = rate * FRAME_SHIFT;

  // Define analyzer
  PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::RECT, 50, 500, u_r1, u_rmax);

  /// \TODO
  /// Preprocess the input signal in order to ease pitch estimation. For instance,
  /// central-clipping or low pass filtering may be used.
  vector<float>::iterator iX;
  //if(preprocess_type == "clip") {
      for (iX = x.begin(); iX != x.end(); ++iX) {
        float amp = *iX;
        if (amp*amp < 0.000001) {
          *iX = 0;
        }
      }
  //}
  //else if(preprocess_type == "lowpass") {

  //}*/


  // Iterate for each frame and save values in f0 vector
  //vector<float>::iterator iX;
  vector<float> f0;
  for (iX = x.begin(); iX + n_len < x.end(); iX = iX + n_shift) {
    float f = analyzer(iX, iX + n_len);
    f0.push_back(f);
  }

  /// \TODO
  /// Postprocess the estimation in order to supress errors. For instance, a median filter
  /// or time-warping may be used.

    //-----------------------------------------
    //El código siguiente proporciona un filtro de media 3 de los valores de f0, pero da un resultado peor
    //por eso, la media por defecto es de 1 (es decir, no hace media)
  vector<float> f0_postprocessed;
  if(median_size % 2 != 0){
  for (long unsigned int i = 0; i < f0.size(); ++i) {
    // Calcular la media de medida median_size
    int half = (median_size-1)/2;
    float sum = 0.0;
    int count = 0;
    for (long unsigned int j = i - half; j <= i + half; ++j) {
        if (j >= 0 && j < f0.size()) {
            sum += f0[j];
            count++;
        }
    }
    float average = sum / count;

    // Agregar el valor postprocesado al nuevo vector
    f0_postprocessed.push_back(average);
}
f0.assign(f0_postprocessed.begin(), f0_postprocessed.end());
  }
//-----------------------------------------------
  


  // Write f0 contour into the output file
  ofstream os(output_txt);
  if (!os.good()) {
    cerr << "Error reading output file " << output_txt << " (" << strerror(errno) << ")\n";
    return -3;
  }

  os << 0 << '\n'; //pitch at t=0
  for (iX = f0.begin(); iX != f0.end(); ++iX) 
    os << *iX << '\n';
  os << 0 << '\n';//pitch at t=Dur

  return 0;
}
