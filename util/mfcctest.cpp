#include "lib/esmeraldamfcc.h"
#include <vector>
#include <iostream>

int main(int argc, char **argv) {
    EsmeraldaMfcc mfcc("");
    std::vector<short> signal(mfcc.signal_bufsize_required(), 10);
    std::vector<float> features(mfcc.feature_bufsize_required());
    std::string params;
    for (int i = 0; i < 10; i++) {
        signal.at(100) = i*10;
        std::cout << "signal: " << signal.size() << std::endl;
        for (auto v : signal) {
            std::cout << v << " ";
        }

        int fcount = mfcc.fextract_frame(signal.data(), features.data());

        std::cout << "features: " << fcount << std::endl;
        for (auto v : features) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
        params = mfcc.get_params();
        std::cout << "params: " << params << std::endl;
    }

}
