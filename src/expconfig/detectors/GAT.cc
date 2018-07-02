#include "GAT.h"
#include "detail/GAT_elements.h"

#include "tree/TID.h"
#include "base/std_ext/math.h"

#include <cassert>
#include <cmath> // provides M_PI

using namespace std;
using namespace ant;
using namespace ant::expconfig::detector;


//maps out time and magnitude of events, only works if all channels are accounted for
void GAT::BuildMappings(vector<UnpackerAcquConfig::hit_mapping_t>& hit_mappings,
                        vector<UnpackerAcquConfig::scaler_mapping_t>&) const
{
    //assigning element as an Element_t (and checking if it's the same as elements?) (both arrays?)
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


//tests if all channels are accounted for
void GAT::InitElements()
{
    assert(elements.size() == 64);
    for(size_t i=0; i<elements.size();i++) {
        Element_t& element = elements[i];
        if(element.Channel != i)
            throw Exception("APT element channels not in correct order");
    }
}
