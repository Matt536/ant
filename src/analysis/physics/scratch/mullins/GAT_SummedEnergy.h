#pragma once

#include "physics/Physics.h"
#include "plot/PromptRandomHist.h"
#include "utils/TriggerSimulation.h"
#include "base/WrapTTree.h"

namespace ant {
namespace analysis {
namespace physics {

class GAT_SummedEnergy : public Physics {

public:
    struct tree_t : WrapTTree
    {
        ADD_BRANCH_T(double,   TaggW)
        ADD_BRANCH_T(unsigned, nClusters)
    };

private:
    TH1D* TotalEnergy;
    //double temp;

    tree_t t;

    PromptRandom::Switch promptrandom;
    utils::TriggerSimulation triggersimu;

public:
    GAT_SummedEnergy(const std::string& name, OptionsPtr opts);

    virtual void ProcessEvent(const TEvent& event, manager_t& manager) override;
    virtual void ShowResult() override;
};

}}}
