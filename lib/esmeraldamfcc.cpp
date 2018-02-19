#include "esmeraldamfcc.h"
#include <vector>
#include <string>

extern "C" {
  #include "dsp.h"
  #include "mfcc_1_1.h"
}

class EsmeraldaMfcc::impl {

private:
    std::vector<char> _channel_params;
    dsp_fextract_t *_fex = nullptr;

public:
    impl(const std::string& channel_params) :
        _channel_params(channel_params.data(), channel_params.data()+channel_params.size()+1u)
    {
        int version = DSP_MK_VERSION(1, 4);
        _fex = dsp_fextract_create(dsp_fextype_MFCC, version, _channel_params.data());
    }

    ~impl() {
        dsp_fextract_destroy(_fex);
    }

    int fextract_frame(dsp_sample_t* signal, mx_real_t* features) {
        int n_features = dsp_fextract_calc(_fex, features, signal);
        return n_features;
    }

    std::string get_params() {
        std::string result;
        char* buf = NULL;
        size_t size = 0;
        FILE* fd = open_memstream(&buf, &size);
        if (fd) {
            char prefix[] = "";
            dsp_fextract_fprintparam(fd, _fex, prefix);
        }
        fflush(fd);
        result.assign(buf, size);
        fclose(fd);
        free(buf);
        return result;
    }

};


EsmeraldaMfcc::EsmeraldaMfcc(const std::string& channel_params) :
    mfcc_impl(new impl(channel_params))
{ }

EsmeraldaMfcc::~EsmeraldaMfcc()
{ }

int EsmeraldaMfcc::fextract_frame(short* signal, float* features)
{
    return mfcc_impl->fextract_frame(signal, features);
}

std::string EsmeraldaMfcc::get_params()
{
    return mfcc_impl->get_params();
}

size_t EsmeraldaMfcc::signal_bufsize_required() {
    return V1_1_FRAME_LEN;
}

size_t EsmeraldaMfcc::feature_bufsize_required() {
    return V1_1_N_FEATURES;
}
