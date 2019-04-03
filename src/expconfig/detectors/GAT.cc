#include "GAT.h"
#include "detail/GAT_elements.h"

#include "tree/TID.h"
#include "base/std_ext/math.h"

#include <cassert>
#include <cmath> // provides M_PI

//Included so that we can call GetGATChannels
//Every new active target setup must be included
#include "setups/Setup_2016_06_Active.h"

using namespace std;
using namespace ant;
using namespace ant::expconfig::detector;


//maps out time and magnitude of events, only works if all channels are accounted for
void GAT::BuildMappings(vector<UnpackerAcquConfig::hit_mapping_t>& hit_mappings,
                        vector<UnpackerAcquConfig::scaler_mapping_t>&) const
{
    //assigning element as an Element_t
    for(const Element_t& element : elements)
    {
        //finds energy
        hit_mappings.emplace_back(Type,
                                  Channel_t::Type_t::Integral,
                                  element.Channel,
                                  element.ADC);
        //finds time
        hit_mappings.emplace_back(Type,
                                  Channel_t::Type_t::Timing,
                                  element.Channel,
                                  element.TDC);
    }
}


void GAT::InitElements()
{
    assert(elements.size() == setup::Setup_2016_06_Active::GetGATChannels());
    for(size_t i=0; i<elements.size();i++) {
        Element_t& element = elements[i];
        if(element.Channel != i)
            throw Exception("GAT element channels not in correct order");
    }
}
