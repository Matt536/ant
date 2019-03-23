#pragma once

#include "base/Detector_t.h"
#include "unpacker/UnpackerAcqu.h"

namespace ant {
namespace expconfig {
namespace detector {
struct GAT :
        Detector_t,
        UnpackerAcquConfig
{


    virtual vec3 GetPosition(unsigned channel) const override {
        return elements[channel].Position;
    }
    virtual unsigned GetNChannels() const override {
        return elements.size();
    }
    virtual void SetElementFlags(unsigned channel, const ElementFlags_t& flags) override {
        elements[channel].Flags |= flags;
    }
    virtual const ElementFlags_t& GetElementFlags(unsigned channel) const override {
        return elements[channel].Flags;
    }

    //for UnpackerAcquConfig (?)
    virtual void BuildMappings(
            std::vector<hit_mapping_t>&,
            std::vector<scaler_mapping_t>&) const override;

protected:
    //declares Element_t under structure Detector_t
    struct Element_t : Detector_t::Element_t {
        //giving Element_t 3 parameters
        Element_t(
                unsigned channel,
                unsigned adc,
                unsigned tdc
                ) :
            Detector_t::Element_t(
                channel,
                vec3(1,0,0)
                ),
            ADC(adc),
            TDC(tdc)
        {}
        unsigned ADC;
        unsigned TDC;
    };

    GAT(const std::vector<Element_t>& elements_init) :
        Detector_t(Detector_t::Type_t::GAT),
        elements(elements_init)
    {
        InitElements();
    }

private:
    bool MaxChannelsReached;
    void InitElements();
    std::vector<Element_t> elements;

};

struct GAT_2017 : GAT {
    GAT_2017() : GAT(elements_init) {}
    static const std::vector<Element_t> elements_init;
};

}}} // namespace ant::expconfig::detector
