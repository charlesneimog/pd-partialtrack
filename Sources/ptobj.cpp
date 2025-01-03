#include "partialtrack.hpp"

static t_class *AnalysisPtr;

// ─────────────────────────────────────
t_PtrPartialAnalysis *newAnalisysPtr(int frameSize, int bufferSize) {
    t_PtrPartialAnalysis *x =
        (t_PtrPartialAnalysis *)getbytes(sizeof(t_PtrPartialAnalysis));
    x->x_pd = AnalysisPtr;
    int sr = sys_getsr();
    x->x_data = new AnalysisData(sr, frameSize, bufferSize);
    std::string PointerStr = std::to_string((long long)x->x_data);
    x->x_sym = gensym(PointerStr.c_str());
    pd_bind((t_pd *)x, x->x_sym);
    return x;
}

// ─────────────────────────────────────
void killAnalisysPtr(t_PtrPartialAnalysis *x) {
    delete x->x_data;
    pd_unbind((t_pd *)x, x->x_sym);
}

// ─────────────────────────────────────
AnalysisData *getAnalisysPtr(t_symbol *s) {
    t_PtrPartialAnalysis *x =
        (t_PtrPartialAnalysis *)pd_findbyclass(s, AnalysisPtr);
    return x ? x->x_data : nullptr;
}
