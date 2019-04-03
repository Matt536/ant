#include "GAT_Plot.h"

using namespace std;
using namespace ant;
using namespace ant::analysis;
using namespace ant::analysis::physics;

// my plot, right now it adds total energy readings
GAT_Plot::GAT_Plot(const string& name, OptionsPtr opts) :
    Physics(name, opts)
{
    BinSettings bins_TotalEnergy(100,0,5);

    TotalEnergy = HistFac.makeTH1D("Total Energy",
                             "Energy","",
                             bins_TotalEnergy,
                             "temp"
                             );

    // define some prompt and random windows (in nanoseconds)
    promptrandom.AddPromptRange({ -7,   7});
    promptrandom.AddRandomRange({-50, -10});
    promptrandom.AddRandomRange({ 10,  50});
    // create/initialize the tree
    t.CreateBranches(HistFac.makeTTree("t"));
}

// process for a SINGLE event
void GAT_Plot::ProcessEvent(const TEvent& event, manager_t&)
{
    double summedEnergy = 0;
    for(auto& hit : event.Reconstructed().DetectorReadHits)
    {
        // screens out data we don't need
        if(hit.DetectorType == Detector_t::Type_t::GAT && hit.ChannelType == Channel_t::Type_t::Integral)
        {
            summedEnergy += hit.Values[0].Calibrated;
        }
    }
    // fills histogram
    TotalEnergy->Fill(summedEnergy);
    t.Tree->Fill();
}

void GAT_Plot::ShowResult()
{
    ant::canvas("Total Energy Histogram")
            << TotalEnergy
            << endc;
}

AUTO_REGISTER_PHYSICS(GAT_Plot)
