#include "pd-partialtrack.hpp"

static t_class *PeaksDetection;
#define MAX_SILENCED_PARTIALS 127

// ==============================================
typedef struct _Peaks {
    t_object xObj;
    t_sample xSample;

    // Multitreading
    bool detached;
    t_sample *audioIn;
    t_int running;
    t_int done;
    t_int firstblock;
    t_int n;

    // clock
    t_clock *x_clock;

    // AnalysisData
    AnalysisData *RealTimeData;
    t_int FrameSize;
    t_int BufferSize;
    t_int maxPeaks;

    t_sample *in;
    t_int audioInBlockIndex;

    // Outlet/Inlet
    t_outlet *sigOut;

} t_Peaks;

static void ConfigPartialTracking(t_Peaks *x, t_symbol *s, int argc,
                                  t_atom *argv) {
    // Sms
    std::string method = x->RealTimeData->PtMethod;
    if (method == "sms") {
        std::string configWhat = atom_getsymbolarg(0, argc, argv)->s_name;

        std::string validMethods[] = {"harmonic"};
        if (std::find(std::begin(validMethods), std::end(validMethods),
                      configWhat) == std::end(validMethods)) {
            pd_error(NULL, "[peaks~] Unknown method for %s SMS PartialTracking",
                     method.c_str());
        }

        if (configWhat == "harmonic") {
            int harmonic = atom_getintarg(1, argc, argv);
            if (harmonic == 0) {
                x->RealTimeData->PtSMS.harmonic(harmonic);
                post("[peaks~] Selected Harmonic PartialTracking without "
                     "phase");
            } else if (harmonic == 1) {
                x->RealTimeData->PtSMS.harmonic(harmonic);
                post("[peaks~] Selected Inharmonic PartialTracking without "
                     "phase");
            } else if (harmonic == 2) {
                x->RealTimeData->PtSMS.harmonic(harmonic);
                post("[peaks~] Selected Harmonic PartialTracking with phase");
            } else if (harmonic == 3) {
                x->RealTimeData->PtSMS.harmonic(harmonic);
                post("[peaks~] Selected Inharmonic PartialTracking with phase");
            } else {
                pd_error(NULL,
                         "[peaks~] Unknown method for %s SMS PartialTracking",
                         method.c_str());
            }
        }
    }
}

// ==============================================
static void SetMethods(t_Peaks *x, t_symbol *sMethod, t_symbol *sName) {
    std::string method = sMethod->s_name;
    std::string name = sName->s_name;
    std::string validMethods[] = {"sms", "loris", "mq", "sndobj"};
    AnalysisData *Anal = (AnalysisData *)x->RealTimeData;

    // check if the method is valid
    if (std::find(std::begin(validMethods), std::end(validMethods), name) ==
        std::end(validMethods)) {
        pd_error(NULL, "[peaks~] Unknown method");
        return;
    }

    if (method == "partial") {
        Anal->PtMethod = name;
    } else if (method == "peak") {
        Anal->PdMethod = name;
    } else {
        pd_error(NULL, "[s-peaks~] This object just define the 'peak' and "
                       "'partial' methods");
        return;
    }
    Anal->error = false;
}

// ==============================================
static void SetDetached(t_Peaks *x, t_float f) {
    if (f == 0) {
        x->detached = false;
    } else {
        x->detached = true;
    }
}

// ==============================================
static void SetConfigs(t_Peaks *x, t_symbol *s, int argc, t_atom *argv) {
    std::string method = atom_getsymbolarg(0, argc, argv)->s_name;
    if (method == "framesize") {
        unsigned int value = atom_getintarg(1, argc, argv);
        x->RealTimeData->set_max_peaks(value);
    } else if (method == "hopsize") {

    } else if (method == "peak") {

    } else if (method == "partial") {

    } else if (method == "harmonic") {

    } else if (method == "fundamental") {
    }

    //

    return;
}

// ==============================================
static void SetMaxPartials(t_Peaks *x, t_float f) {
    AnalysisData *Anal = (AnalysisData *)x->RealTimeData;
    Anal->set_max_peaks(f);
}

// ==============================================
static void PeaksTick(t_Peaks *x) {
    t_atom args[1];

    char ptr[MAXPDSTRING];
    sprintf(ptr, "%p", x->RealTimeData);
    SETSYMBOL(&args[0], gensym(ptr));
    outlet_anything(x->sigOut, gensym("simplObj"), 1, args);

    x->RealTimeData->Frame.clear_peaks();
    x->RealTimeData->Frame.clear_partials();
    x->RealTimeData->Frame.clear_synth();
}

// ==============================================
static void PartialTrackingProcessor(t_Peaks *x) {
    DEBUG_PRINT("[peaks~] Start PartialTracking");

    AnalysisData *Anal = (AnalysisData *)x->RealTimeData;

    Anal->mtx.lock();

    float *in = (float *)x->in;
    std::copy(in, in + x->BufferSize, Anal->audio.begin());
    Anal->Frame.audio(&(Anal->audio[0]), x->BufferSize);
    Anal->PeakDectection();
    Anal->PartialTracking();
    Anal->mtx.unlock();

    clock_delay(x->x_clock, 0);
    DEBUG_PRINT("[peaks~] Finished PartialTracking");
}

// ==============================================
static t_int *PeaksAudioPerform(t_int *w) {
    t_Peaks *x = (t_Peaks *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    x->n = n;
    for (int i = 0; i < n; i++) {
        x->in[x->audioInBlockIndex] = (double)in[i];
        x->audioInBlockIndex++;
        if (x->audioInBlockIndex == x->BufferSize) {
            x->audioInBlockIndex = 0;
            PartialTrackingProcessor(x);
        }
    }
    return (w + 4);
}

// ==============================================
static void PeaksAddDsp(t_Peaks *x, t_signal **sp) {
    DEBUG_PRINT("[peaks~] Adding Dsp");
    dsp_add(PeaksAudioPerform, 3, x, sp[0]->s_vec, (t_int)sp[0]->s_n);
    DEBUG_PRINT("[peaks~] Dsp Rotine added");
}

// ==============================================
static void *NewPeaks(t_symbol *s, int argc, t_atom *argv) {
    t_Peaks *x = (t_Peaks *)pd_new(PeaksDetection);
    x->sigOut = outlet_new(&x->xObj, &s_anything);
    x->x_clock = clock_new(x, (t_method)PeaksTick);

    x->maxPeaks = 127; // TODO: No default
    int hopSize = 1024;

    x->FrameSize = 2048;
    x->BufferSize = 512;

    bool hopDefined = false;
    bool methodDefined = false;

    std::string pd = "";
    std::string pt = "";
    std::string sy = "";

    // search for -pd, -pt, -sy -hop
    for (int i = 0; i < argc; i++) {
        if (argv[i].a_type == A_SYMBOL) {
            std::string arg = atom_getsymbolarg(i, argc, argv)->s_name;
            t_symbol *s = atom_getsymbolarg(i, argc, argv);
            if (arg == "-pd") {
                i++;
                pd = atom_getsymbolarg(i, argc, argv)->s_name;
            } else if (arg == "-pt") {
                i++;
                pt = atom_getsymbolarg(i, argc, argv)->s_name;
            } else if (arg == "-sy") {
                i++;
                sy = atom_getsymbolarg(i, argc, argv)->s_name;
            } else if (arg == "-fr") {
                i++;
                x->FrameSize = atom_getfloatarg(i, argc, argv);
                hopDefined = true;
            } else if (arg == "-hop") {
                i++;
                x->BufferSize = atom_getfloatarg(i, argc, argv);
                hopDefined = true;
            }
        }
    }

    if (x->FrameSize < x->BufferSize) {
        pd_error(NULL,
                 "[peaks~] Frame size [-fr] must be greater than the hopsize");
        return NULL;
    }

    if (!hopDefined) {
        post("[peaks~] Using default FrameSize of 1024 and HopSize of 512 "
             "samples");
        x->FrameSize = 1024;
        x->BufferSize = 512;
    }

    // partials
    x->in = new t_sample[x->BufferSize];
    x->done = 0;

    // analisys
    static AnalysisData Anal(x->FrameSize, x->BufferSize);
    Anal.PdMethod = pd;
    Anal.PtMethod = pt;
    Anal.SyMethod = sy;

    x->RealTimeData = &Anal;
    DEBUG_PRINT("[peaks~] Object created");
    return (void *)x;
}

// ==============================================
void PeakSetup(void) {
    PeaksDetection = class_new(gensym("pt-peaks~"), (t_newmethod)NewPeaks, NULL,
                               sizeof(t_Peaks), CLASS_DEFAULT, A_GIMME, 0);

    CLASS_MAINSIGNALIN(PeaksDetection, t_Peaks, xSample);
    class_addmethod(PeaksDetection, (t_method)PeaksAddDsp, gensym("dsp"),
                    A_CANT, 0);
    class_addmethod(PeaksDetection, (t_method)SetDetached, gensym("detached"),
                    A_FLOAT, 0);
    class_addmethod(PeaksDetection, (t_method)SetConfigs, gensym("set"),
                    A_GIMME, 0);
    class_addmethod(PeaksDetection, (t_method)ConfigPartialTracking,
                    gensym("ptcfg"), A_GIMME, 0);

    //
}
