#pragma once

#include "Calibration.h"
#include "base/interval.h"

class TGraph;

namespace ant {

namespace expconfig {
namespace detector {
struct TaggerDetector_t;
}}

namespace calibration {

class DataManager;


class TaggEff : public Calibration::Module
{
public:
    TaggEff(const std::shared_ptr<expconfig::detector::TaggerDetector_t>&  tagger,
            const std::shared_ptr<DataManager>& calmgr
            );
    virtual ~TaggEff();

    virtual void GetGUIs(std::list<std::unique_ptr<calibration::gui::CalibModule_traits> >&) override;

    // Updateable_traits interface
    virtual std::list<Loader_t> GetLoaders() override;

protected:
    std::shared_ptr<expconfig::detector::TaggerDetector_t> pid_detector;
    std::shared_ptr<DataManager> calibrationManager;
};

}}