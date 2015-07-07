#ifndef GOATREADER_H
#define GOATREADER_H

#include "input/DataReader.h"
#include "data/Event.h"

#include <memory>
#include <string>
#include <list>

#include "detail/FileManager.h"
#include "detail/TreeManager.h"
#include "detail/InputModule.h"

#include "detail/TriggerInput.h"
#include "detail/TaggerInput.h"
#include "detail/DetectorHitInput.h"
#include "detail/TrackInput.h"
#include "detail/PlutoInput.h"
#include "detail/ParticleInput.h"


class PStaticData;


namespace ant {
namespace input {

class GoatReader: public DataReader {
protected:

    class InputWrapper {

    };

    class ModuleManager: public std::list<BaseInputModule*> {
    public:
        ModuleManager() = default;
        ModuleManager(const std::initializer_list<BaseInputModule*>& initlist):
            std::list<BaseInputModule*>(initlist) {}

        void GetEntry() {
            for(auto& module : *this) {
                module->GetEntry();
            }
        }
    };

    FileManager   files;
    TreeManager   trees;


    TriggerInput        trigger;
    TaggerInput         tagger;
    TrackInput          tracks;
    DetectorHitInput    detectorhits;
    PlutoInput          pluto;
    ParticleInput       photons   = ParticleInput("photons");
    ParticleInput       protons   = ParticleInput("protons");
    ParticleInput       pichagred = ParticleInput("pions");
    ParticleInput       echarged  = ParticleInput("echarged");
    ParticleInput       neutrons  = ParticleInput("neutrons");

    ModuleManager active_modules = {
        &trigger,
        &tagger,
        &tracks,
        &detectorhits,
        &pluto,
        &photons,
        &protons,
        &pichagred,
        &echarged,
        &neutrons
    };

    Long64_t    current_entry = -1;

    void AddInputModule(BaseInputModule& module);

    static clustersize_t MapClusterSize(const int& size);

    void CopyTagger(std::shared_ptr<Event>& event);
    void CopyTrigger(std::shared_ptr<Event>& event);
    void CopyDetectorHits(std::shared_ptr<Event>& event);
    void CopyTracks(std::shared_ptr<Event>& event);
    void CopyPluto(std::shared_ptr<Event>& event);
    void CopyParticles(std::shared_ptr<Event>& event, ParticleInput& input_module, const ParticleTypeDatabase::Type& type);

    PStaticData* pluto_database;

public:
    GoatReader();
    virtual ~GoatReader() = default;

    void AddInputFile(const std::string& filename);
    void Initialize();

    /**
     * @brief Get number of events in tree
     * @see TotalEvents()
     * @return number of events total
     */
    Long64_t  GetNEvents() const;

    std::shared_ptr<Event> ReadNextEvent();
    bool hasData() const;

    long long EventsRead() const override;
    long long TotalEvents() const override;
};

}
}

#endif
