#pragma once

#define MAX_SILENCED_PARTIALS 127

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <simpl.h> // TODO: Need to fix this
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

class TransParams {
  public:
    float sMidi[MAX_SILENCED_PARTIALS];
    float sVariation[MAX_SILENCED_PARTIALS];
    unsigned int sIndex;

    // Transpose Partials
    float tMidi[MAX_SILENCED_PARTIALS];
    float tVariation[MAX_SILENCED_PARTIALS]; // cents
    float tCents[MAX_SILENCED_PARTIALS];
    unsigned int tIndex;

    // Change Transpose all
    float all_cents;
    bool all_exec;

    // Change Amps
    float aMidi[MAX_SILENCED_PARTIALS];
    float aVariation[MAX_SILENCED_PARTIALS]; // cents
    float aAmpsFactor[MAX_SILENCED_PARTIALS];
    unsigned int aIndex;

    // Change Transpose all

    // Expand Partials
    float eFactor;
    float eFundFreq;
    float ePrevFreq;

    void transpose(simpl::Peak *Peak);
    void transposeall(simpl::Peak *Peak);
    void changeamps(simpl::Peak *Peak);
    void silence(simpl::Peak *Peak);
    void expand(simpl::Peak *Peak);
    void reset();
};

// Helpers
double f2m(double f);
double m2f(double m);
double transFreq(double originalFrequency, double cents);
