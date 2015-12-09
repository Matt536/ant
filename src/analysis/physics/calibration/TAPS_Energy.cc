#include "TAPS_Energy.h"

#include "utils/combinatorics.h"

#include "expconfig/ExpConfig.h"
#include "expconfig/detectors/TAPS.h"

using namespace std;
using namespace ant;
using namespace ant::analysis::data;
using namespace ant::analysis::physics;



TAPS_Energy::TAPS_Energy(const string& name, analysis::PhysOptPtr opts) :
    Physics(name, opts)
{
    taps_detector = ExpConfig::Setup::GetDetector<expconfig::detector::TAPS>();

    const BinSettings taps_channels(taps_detector->GetNChannels());
    const BinSettings energybins(1000);
    const BinSettings timebins(1000,-100,100);

    ggIM = HistFac.makeTH2D("2 neutral IM (TAPS,CB)", "IM [MeV]", "#",
                            energybins, taps_channels, "ggIM");

    timing_cuts = HistFac.makeTH2D("Check timing cuts", "IM [MeV]", "#",
                                   timebins, taps_channels, "timing_cuts");

    h_pedestals = HistFac.makeTH2D(
                      "TAPS Pedestals",
                      "Raw ADC value",
                      "#",
                      BinSettings(300),
                      taps_channels,
                      "Pedestals");
}

void TAPS_Energy::ProcessEvent(const Event& event)
{
    const auto& cands = event.Reconstructed.Candidates;

    // pedestals
    for(const Cluster& cluster : event.Reconstructed.AllClusters) {
        if(!(cluster.Detector & Detector_t::Type_t::TAPS))
            continue;
        for(const Cluster::Hit& clusterhit : cluster.Hits) {
            /// \todo check for timing hit?
            /// \todo check for trigger pattern?
            for(const Cluster::Hit::Datum& datum : clusterhit.Data) {
                if(datum.Type != Channel_t::Type_t::Pedestal)
                    continue;
                h_pedestals->Fill(datum.Value, clusterhit.Channel);
            }
        }
    }

    const auto CBTAPS = Detector_t::Type_t::CB | Detector_t::Type_t::TAPS;

    for( auto comb = analysis::utils::makeCombination(cands,2); !comb.Done(); ++comb ) {
        const CandidatePtr& cand1 = comb.at(0);
        const CandidatePtr& cand2 = comb.at(1);

        if(cand1->VetoEnergy==0 && cand2->VetoEnergy==0) {

            //require exactly 1 CB and 1 TAPS
            const auto dets = (cand1->Detector & CBTAPS) ^ (cand2->Detector & CBTAPS);

            if(dets & CBTAPS) {
                const Particle a(ParticleTypeDatabase::Photon,comb.at(0));
                const Particle b(ParticleTypeDatabase::Photon,comb.at(1));
                const TLorentzVector& gg = a + b;


                // Find the one that was in TAPS
                auto cand_taps = cand1->Detector & Detector_t::Type_t::TAPS ? cand1 : cand2;
                auto cl_taps = cand_taps->FindCaloCluster();
                if(cl_taps) {
                    const unsigned ch = cl_taps->CentralElement;
                    const unsigned ring = taps_detector->GetRing(ch);

                    // fill in IM only if ring>4 or if timecut is passed
                    double weight = -1.0;
                    if(ring > 4 || fabs(cand_taps->Time) < 2.5) {
                        weight = 1.0;
                        ggIM->Fill(gg.M(),ch);
                    }
                    timing_cuts->Fill(cand_taps->Time, ch, weight);
                }
            }
        }
    }
}

void TAPS_Energy::ShowResult()
{
    canvas(GetName()) << drawoption("colz") << ggIM
                      << drawoption("colz") << timing_cuts
                      << drawoption("colz") << h_pedestals
                      << endc;
}

AUTO_REGISTER_PHYSICS(TAPS_Energy)
