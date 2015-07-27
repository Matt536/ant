#pragma once

#include "Calibration.h"
#include "expconfig/Detector_t.h"

#include "tree/TDataRecord.h" // for TKeyValue, TID

#include <memory>
#include <limits>

class TH1;

namespace ant {
namespace calibration {

class Timing :
        public Calibration::Module,
        public ReconstructHook::DetectorReadHits
{

public:

    Timing(
            Detector_t::Type_t DetectorType,
            Calibration::Converter::ptr_t converter,
            double defaultOffset,
            const interval<double>& timeWindow = {-std_ext::inf, std_ext::inf},
            double defaultGain = 1.0, // default gain is 1.0
            const std::vector< TKeyValue<double> >& gains = {}
            );

    // ReconstructHook
    virtual void ApplyTo(const readhits_t& hits, extrahits_t&) override;

    // Updateable_traits interface
    virtual std::list<TID> GetChangePoints() const override;
    void Update(const TID&) override;

protected:

    const Detector_t::Type_t DetectorType;
    const Calibration::Converter::ptr_t Converter;

    const interval<double> TimeWindow;

    const double DefaultOffset;
    std::vector<double> Offsets;

    const double DefaultGain;
    std::vector<double> Gains;

};

}}  // namespace ant::calibration
