#include "calibration/DataManager.h"
#include "expconfig/ExpConfig.h"

#include "base/Logger.h"
#include "tclap/CmdLine.h"
#include "base/std_ext/string.h"
#include "base/OptionsList.h"
#include "base/WrapTFile.h"
#include "TH2.h"
#include "TH3.h"
#include "root-addons/analysis_codes/TwoPi0_MCSmearing_tools.h"
#include "TROOT.h"
#include "TRint.h"
#include "tree/TCalibrationData.h"
#include "calibration/modules/detail/TH2Storage.h"
#include "base/TH_ext.h"
#include <iostream>
#include <cstring>
#include "TDirectory.h"
#include "base/std_ext/string.h"
#include "analysis/plot/RootDraw.h"
#include "base/Array2D.h"

using namespace std;
using namespace ant;
using namespace ant::calibration::gui;

TH2D* GetHist(WrapTFileInput& file, const string& histname) {
    TH2D* h = nullptr;
    file.GetObject(std_ext::formatter() << "ETheta/" << histname, h);
    if(!h) {
        LOG(ERROR) << "ETheta/" << histname << " not found in " << file.FileNames();
        exit(EXIT_FAILURE);
    }
    return h;
}

inline string ToUpper(const string& s) {
    string out = s;
    transform(s.begin(), s.end(),out.begin(), ::toupper);
    return out;
}

inline string ToLower(const string& s) {
    string out = s;
    transform(s.begin(), s.end(),out.begin(), ::tolower);
    return out;
}

int main(int argc, char** argv) {
    SetupLogger();


    TCLAP::CmdLine cmd("Ant-mcsmearing", ' ', "0.1");
    auto cmd_verbose = cmd.add<TCLAP::ValueArg<int>>("v","verbose","Verbosity level (0..9)", false, 0,"level");
    auto cmd_batchmode = cmd.add<TCLAP::SwitchArg>("b","batch","Run in batch mode (no GUI, autosave)",false);
    auto cmd_setupname = cmd.add<TCLAP::ValueArg<string>>("s","setup","Setup name", false, "", "setup");

    auto cmd_detector  = cmd.add<TCLAP::ValueArg<string>>("","detector","Detector Name",         true, "", "detector");

    auto cmd_fit  = cmd.add<TCLAP::ValueArg<string>>("f","fit","Fit peaks in IM spectra",         false, "", "fit");
    auto cmd_step = cmd.add<TCLAP::SwitchArg>("","step", "Calculate next iteration step. Init if no previous data in calib database", false);
    auto cmd_compare = cmd.add<TCLAP::SwitchArg>("","compare", "Compare peak width of data and mc", false);
    auto cmd_data = cmd.add<TCLAP::ValueArg<string>>("d","data","Histogram file for data",         false, "", "data");
    auto cmd_mc   = cmd.add<TCLAP::ValueArg<string>>("m","mc",  "Current iterarion mc histograms", false, "", "mc");
    auto cmd_write   = cmd.add<TCLAP::SwitchArg>("","write",  "Write to database");
    cmd.parse(argc, argv);

    if(cmd_verbose->isSet())
        el::Loggers::setVerboseLevel(cmd_verbose->getValue());

    if(!cmd_detector->isSet()) {
        LOG(ERROR) << "Detector not set";
        return EXIT_FAILURE;
    }

    const auto det = ToLower(cmd_detector->getValue());

    shared_ptr<calibration::DataManager> manager = nullptr;

    unique_ptr<WrapTFileOutput> outfile = nullptr;

    // create TRint app early in order to have valid gStyle pointer...
    int fake_argc=1;
    char* fake_argv[2];
    fake_argv[0] = argv[0];
    if(cmd_batchmode->isSet()) {
        fake_argv[fake_argc++] = strdup("-q");
    }
    auto app = new TRint("Ant-mcsmearing",&fake_argc,fake_argv,nullptr,0,true);

    if(cmd_fit->isSet() && !cmd_step->isSet() && !cmd_compare->isSet()) {
        // FIT
        TH3* h_ETheta = nullptr;
        WrapTFileInput infile(cmd_fit->getValue());
        const string histname = std_ext::formatter() << "TwoPi0_MCSmearing/" << det << "_pi0_ETheta";
        infile.GetObject(histname, h_ETheta);

        if(!h_ETheta) {
            LOG(ERROR) << "Histogram " << histname << " not found!" << endl;
            return EXIT_FAILURE;
        }

        outfile =  std_ext::make_unique<WrapTFileOutput>( std_ext::formatter() << cmd_fit->getValue() << "." << det << "_fitted.root", true);

        auto d = gDirectory->mkdir("ETheta");
        d->cd();
        ant::TwoPi0_MCSmearing_Tool::AnalyseChannelE(h_ETheta);

    }
    else if(!cmd_fit->isSet() && ( cmd_compare->isSet() || cmd_step->isSet())) {

        WrapTFileInput datafile(cmd_data->getValue());
        WrapTFileInput mcfile(cmd_mc->getValue());

        const auto data_width = GetHist(datafile, "sigma");
        const auto mc_width   = GetHist(mcfile,   "sigma");
        if(!cmd_step->isSet()) {

            canvas("Compare") << drawoption("colz") << TH_ext::Clone(data_width,"data") << TH_ext::Clone(mc_width,"mc") << TwoPi0_MCSmearing_Tool::THDataMCDiff(mc_width,data_width,"datamc") << endc;

        } else if(cmd_step->isSet()) {

            const auto setup_name = cmd_setupname->getValue();

            ExpConfig::Setup::SetByName(setup_name);
            auto& setup = ExpConfig::Setup::Get();

            manager = setup.GetCalibrationDataManager();

            manager->SetOverrideToDefault(true);

            const auto id = TID(0,0,{TID::Flags_t::MC});

            TID next;

            // smearing
            {

                const string calName = std_ext::formatter() << ToUpper(det) << "_ClusterSmearing";
                TCalibrationData prev_data;
                const auto prev_avail = manager->GetData(calName, id, prev_data, next);

                TH2D* smearing = nullptr;

                if(prev_avail) {
                    LOG(INFO) << "Calculating next iteration...";
                    auto prev_hist = calibration::detail::TH2Storage::Decode(prev_data);
                    smearing = TwoPi0_MCSmearing_Tool::CalculateUpdatedSmearing(data_width,mc_width, prev_hist);
                } else {
                    LOG(INFO) << "Calculating initial smearing...";
                    smearing = TwoPi0_MCSmearing_Tool::CalculateInitialSmearing(data_width, mc_width);
                }

                /// \todo check if IQR factors are well-chosen
                Array2D_TH2D(smearing).RemoveOutliers(std_ext::inf, 3);

                TCalibrationData cdata(calName, id, id);
                calibration::detail::TH2Storage::Encode(smearing, cdata);

                if(cmd_write->isSet())
                    manager->Add(cdata,  Calibration::AddMode_t::AsDefault);

                canvas("Smearing") << drawoption("text") << TH_ext::Clone(data_width, "data") << TH_ext::Clone(mc_width,"mc") << smearing << endc;
            }

        }


    }
    app->Run(kTRUE);
    ExpConfig::Setup::Cleanup();
    manager = nullptr;
    outfile = nullptr;

    return EXIT_SUCCESS;
}
