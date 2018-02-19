#pragma once

#include <string>
#include <memory>
#include <vector>

class EsmeraldaMfcc
{
public:
    /** params consists of
    * a vad-threshold (0.0-1.0)
    * a low and high energy: e_low, e_high,
    * a mfcc mean vector: E{energy}, E{C_1} ... E{C_12}
    */
    EsmeraldaMfcc(const std::string& channel_params = "");

    /**
     * @brief fextract_frame
     * @param signal (16 kHz 16 bit signed integer, 16ms length,
     * typically the window is shifted 10ms per iteration)
     * @param features
     * @return number of features extracted (0 for the first few frames due to
     * the window required for calculating the derivatives)
     */
    int fextract_frame(short *signal, float *features);
    std::string get_params();
    size_t signal_bufsize_required();
    size_t feature_bufsize_required();
    ~EsmeraldaMfcc();
private:
    class impl;
    std::unique_ptr<impl> mfcc_impl;
};
