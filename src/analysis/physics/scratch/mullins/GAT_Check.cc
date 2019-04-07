#include "GAT_Check.h"

using namespace std;
using namespace ant;
using namespace ant::analysis;
using namespace ant::analysis::physics;


GAT_Check::GAT_Check(const string& name, OptionsPtr opts) :
    Physics(name, opts)
{
    BinSettings bins_atT(100,0,15);
    BinSettings bins_atE(100,0,15);
    BinSettings bins_atI(6,0,6);


    h_atI = HistFac.makeTH1D("atI",
                             "atI","",
                             bins_atI,
                             "htempI"
                             );

    h_atE = HistFac.makeTH1D("atE",
                             "atE","",
                             bins_atE,
                             "htempE"
                             );

    h_atT = HistFac.makeTH1D("atT",
                             "atT","",
                             bins_atT,
                             "htempT"
                             );

    // define some prompt and random windows (in nanoseconds)
    promptrandom.AddPromptRange({ -7,   7});
    promptrandom.AddRandomRange({-50, -10});
    promptrandom.AddRandomRange({ 10,  50});

    // create/initialize the tree
    t.CreateBranches(HistFac.makeTTree("t"));

}

// adds data
void GAT_Check::ProcessEvent(const TEvent& event, manager_t&)
{
    for(auto& hit : event.Reconstructed().DetectorReadHits)
    {
        if(hit.DetectorType == Detector_t::Type_t::GAT){
            if(hit.ChannelType == Channel_t::Type_t::Integral)h_atE->Fill(hit.Values[0].Calibrated);
            if(hit.ChannelType == Channel_t::Type_t::Timing)
            {
                h_atT->Fill(hit.Values[0].Calibrated);
                h_atI->Fill(hit.Channel);
            }
            t.Tree->Fill();
        }
    }
}

void GAT_Check::ShowResult()
{
    ant::canvas("atI, atE, and atT plots")
            << h_atI
            << h_atE
            << h_atT
            << endc;
}


AUTO_REGISTER_PHYSICS(GAT_Check)
