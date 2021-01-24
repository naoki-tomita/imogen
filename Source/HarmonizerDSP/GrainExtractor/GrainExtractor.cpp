/*
  ==============================================================================

    GrainExtractor.cpp
    Created: 23 Jan 2021 2:38:27pm
    Author:  Ben Vining

  ==============================================================================
*/

#include "GrainExtractor.h"


template<typename SampleType>
GrainExtractor<SampleType>::GrainExtractor()
{ };


template<typename SampleType>
GrainExtractor<SampleType>::~GrainExtractor()
{ };


template<typename SampleType>
void GrainExtractor<SampleType>::prepare (const int maxBlocksize)
{
    // maxBlocksize = max period of input audio
    
    peakIndices.ensureStorageAllocated (maxBlocksize);
    
    prepareForPsola (maxBlocksize);
};


template<typename SampleType>
void GrainExtractor<SampleType>::releaseResources()
{
    peakIndices.clear();
    
    releasePsolaResources();
};


template<typename SampleType>
void GrainExtractor<SampleType>::getGrainOnsetIndicesForUnpitchedAudio (Array<int>& targetArray,
                                                                        const AudioBuffer<SampleType>& inputAudio,
                                                                        const int grainRate)
{
    targetArray.clearQuick();
    
    targetArray.add (0);
    
    for (int i = grainRate;
         i < inputAudio.getNumSamples();
         i += grainRate)
        targetArray.add (i);
};



template<typename SampleType>
void GrainExtractor<SampleType>::getGrainOnsetIndices (Array<int>& targetArray,
                                                       const AudioBuffer<SampleType>& inputAudio,
                                                       const int period)
{
    targetArray.clearQuick();
    peakIndices.clearQuick();
    
    const int totalNumSamples = inputAudio.getNumSamples();
    
    // PART ONE -- identify sample indices of points of interest for synchronicity, whether it's points of maximum energy every pitch period (PSOLA), or epochs, etc
    // regardless of synchronicity / peak picking algorithm used, the indices of the points of interest should be placed into the peakIndices array
    
    // identifies peak indices for each pitch period & places them in the peakIndices array
    findPsolaPeaks (peakIndices, inputAudio.getReadPointer(0), totalNumSamples, period);
    
    jassert (! peakIndices.isEmpty());
    
    
    // PART TWO - create array of grain onset indices, such that grains are 2 pitch periods long, CENTERED on points of synchronicity previously identified
    // in order to do this, such that the output grains are centered on our indices from peakIndices, we subtract from each index half the length of the output analysis grains -- which in this case, is the period, because our grains are 2 periods long.
    
    for (int p = 0; p < peakIndices.size(); ++p)
    {
        const int peakIndex = peakIndices.getUnchecked (p);
        
        int grainStart = peakIndex - period; // offset the peak index by the period so that the peakIndex will be in the center of the grain
        
        if (grainStart >= 0)
            targetArray.add (grainStart);
    }
    
    // fill in hypothetial missed grains @ start of audio
    int first = targetArray.getUnchecked(0);
    first -= period;
    
    while (first >= 0)
    {
        if (! targetArray.contains (first))
            targetArray.insert (0, first);
        
        first -= period;
    }
    
    // fill in hypothetical missed grains @ end of audio
    int last = targetArray.getLast();
    last += period;
    
    while (last < inputAudio.getNumSamples())
    {
        if (! targetArray.contains (last))
            targetArray.add (last);
        
        last += period;
    }
};


template<typename SampleType>
bool GrainExtractor<SampleType>::isPeriodRepresentedByAPeak (const Array<int>& peaks,
                                                             const int periodMinSample, const int periodMaxSample)
{
    for (int s = periodMinSample; s < periodMaxSample; ++s)
        if (peaks.contains(s))
            return true;
    
    return false;
};


template class GrainExtractor<float>;
template class GrainExtractor<double>;
