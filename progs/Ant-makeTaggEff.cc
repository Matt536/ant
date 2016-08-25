#include <assert.h>

#include "analysis/plot/HistogramFactories.h"

#include "base/std_ext/system.h"
#include "base/Logger.h"
#include "base/CmdLine.h"
#include "base/WrapTFile.h"
#include "base/std_ext/string.h"

#include "expconfig/setups/Setup.h"
#include "expconfig/ExpConfig.h"

#include "analysis/plot/root_draw.h"

#include "TTree.h"
#include "TRint.h"
#include "TH1D.h"

#include "calibration/Editor.h"
#include "calibration/DataManager.h"

#include "analysis/physics/common/ProcessTaggEff.h"

#include "tree/TAntHeader.h"

using namespace ant;
using namespace std;
using namespace ant::analysis;
using namespace ant::std_ext;
static volatile bool interrupt = false;


class taggEffTriple_t
{
protected:
    struct treeContainer_t
    {
        WrapTFileInput wrapFile;
        physics::ProcessTaggEff::TreeScalarReads wrapTree;

        string setupName;
        size_t nchannels;


        treeContainer_t(const string& filename):
            wrapFile(filename)
        {
            auto treeName = physics::ProcessTaggEff::treeAccessName();
            if (!wrapFile.GetObject(treeName,wrapTree.Tree))
            {
                LOG(ERROR) << "Cannot find tree " << treeName
                           << " in file " << filename;
                exit(EXIT_FAILURE);
            }

            wrapTree.LinkBranches(wrapTree.Tree);

            ant::TAntHeader* h;
            wrapFile.GetObject("AntHeader",h);
            setupName = h->SetupName;
            ExpConfig::Setup::SetManualName(setupName);
            nchannels = ExpConfig::Setup::GetDetector<TaggerDetector_t>()->GetNChannels();
        }

        struct means_t {
            vector<double> electrons;
            vector<double> livetime;
            vector<double> tdcs;
        };

        means_t getMeans() const
        {
            means_t means;
            means.electrons.resize(nchannels);
            means.livetime.resize(nchannels);
            means.tdcs.resize(nchannels);


            vector<double> electrons;
            for (auto entry = 0 ; entry < wrapTree.Tree->GetEntries() ; ++entry)
            {
                wrapTree.Tree->GetEntry(entry);
                for ( auto channel = 0u ; channel < wrapTree.TaggRates().size(); ++channel)
                {
                    electrons.at(channel) += 1.0 * wrapTree.TaggRates().at(channel);
                }
            }
            return means;
        }

    };

    treeContainer_t bkg1;
    treeContainer_t run;
    treeContainer_t bkg2;



public:

    taggEffTriple_t(const string& bkg1f_, const string& runf_, const string& bkg2f_):
        bkg1(bkg1f_),
        run(runf_),
        bkg2(bkg2f_)
    {

        if ( !(bkg1.setupName == bkg2.setupName &&
             bkg2.setupName == run.setupName ) )
        {
            LOG(ERROR) << "Files in TaggEff-triple not from same Setup!";
            exit(EXIT_FAILURE);
        }



        cout << "bkg1  " << bkg1.wrapTree.Tree->GetEntries() << endl;
        cout << "run  " << run.wrapTree.Tree->GetEntries() << endl;
        cout << "bkg2  " << bkg2.wrapTree.Tree->GetEntries() << endl;

    }

    string SetupName() const{ return bkg1.setupName;}

    // TODO: calibration data
    vector<double> const GetTaggEff()
    {
        const auto nchannels = ExpConfig::Setup::GetDetector<TaggerDetector_t>()->GetNChannels();
        vector<double> electrons(nchannels,0);
        vector<double> electronBkg(nchannels,0);

        for (auto entry = 0 ; entry < run.wrapTree.Tree->GetEntries() ; ++entry)
        {
            run.wrapTree.Tree->GetEntry(entry);
            for ( auto channel = 0u ; channel < nchannels ; ++channel)
            {
                electrons.at(channel) += 1.0 * run.wrapTree.TaggRates().at(channel);
            }
        }

        for (auto entry = 0 ; entry < bkg1.wrapTree.Tree->GetEntries() ; ++entry)
        {
            bkg1.wrapTree.Tree->GetEntry(entry);
            for ( auto channel = 0u ; channel < nchannels ; ++channel)
            {
                electronBkg.at(channel) += bkg1.wrapTree.TaggRates().at(channel);
            }
        }

        for (auto entry = 0 ; entry < bkg2.wrapTree.Tree->GetEntries() ; ++entry)
        {
            bkg2.wrapTree.Tree->GetEntry(entry);
            for ( auto channel = 0u ; channel < nchannels ; ++channel)
            {
                electronBkg.at(channel) += bkg2.wrapTree.TaggRates().at(channel);
            }
        }

        vector<double> result(nchannels,0);
        for (auto channel = 0u ; channel < nchannels ; ++channel)
        {
            result.at(channel) = electrons.at(channel) + ( electronBkg.at(channel) / 2.0 );
        }


        return result;
    }

};


string processFiles(const string& bkg1, const string& run, const string& bkg2)
{

    taggEffTriple_t taggEff(bkg1,run,bkg2);

    cout << "TaggEff by channel:" << endl;
    for (const auto eff: taggEff.GetTaggEff())
        cout << eff << "  ";
    cout << endl;




    return taggEff.SetupName(); // any is ok, it is checked
}

bool processCSV(const string& csvFile)
{
    ifstream csvStream(csvFile);
    if (!csvStream)
    {
        LOG(ERROR) << "Error reading File list " << csvFile << ".";
        return false;
    }

    string setupName;
    size_t n_TaggEffs(0);

    while (csvStream)
    {
        string line;
        if (!getline(csvStream, line))
        {
            LOG(INFO) << "Done reading File list " << csvFile << ".";
            break;
        }


        istringstream sstream(line);
        vector<string> record;

        while (sstream)
        {
            string s;
            if (!getline(sstream,s,','))
                break;
            record.emplace_back(s);
        }

        if (record.size() != 3)
        {
            LOG(ERROR) << "Found line with wrong number of files, check your file list.";
            return false;
        }

        string currentSetup(processFiles(record[0],record[1],record[2]));
        if (n_TaggEffs > 0)
            if (currentSetup != setupName)
            {
                LOG(ERROR) << "Different Setupnames within file list found!";
                return false;
            }
        setupName = currentSetup;
        n_TaggEffs++;
    }

    return true;
}


int main( int argc, char** argv )
{
    SetupLogger();

    signal(SIGINT, [] (int) {
        LOG(INFO) << ">>> Interrupted";
        interrupt = true;
    });

    TCLAP::CmdLine cmd("Ant-makeSigmas", ' ', "0.1");

    auto cmd_input        = cmd.add<TCLAP::ValueArg<string>>("i","input",      "CSV file with bkg1-run-bkg2 groups",             false,  "", "csv");
    auto cmd_bkg1         = cmd.add<TCLAP::ValueArg<string>>("f","first-bkg",  "input for first background",                     false,  "", "root");
    auto cmd_taggEff      = cmd.add<TCLAP::ValueArg<string>>("t","taggEff",    "TaggEff - run",                                  false,  "", "root");
    auto cmd_bkg2         = cmd.add<TCLAP::ValueArg<string>>("s","second-bkg", "input for second background",                    false,  "", "root");


    auto cmd_output       = cmd.add<TCLAP::ValueArg<string>>("o","",           "output-file",                                    false, "", "rootfile");

    auto cmd_batchmode    = cmd.add<TCLAP::MultiSwitchArg>  ("b","batch",      "Run in batch mode (no ROOT shell afterwards)",   false);


    cmd.parse(argc, argv);

    const auto inCsv   = cmd_input->getValue();


    const auto outfile = cmd_output->getValue();


    if (cmd_input->isSet())
    {
        if ( cmd_bkg1->isSet()    ||
             cmd_bkg2->isSet()    ||
             cmd_taggEff->isSet()    )
        {
            LOG(ERROR) << "Provide either csv file list or triple of backgrounds and TaggEff run!";
            return EXIT_FAILURE;
        }
        if (processCSV(inCsv))
            return EXIT_SUCCESS;
    }

    const auto inBkg1    = cmd_bkg1->getValue();
    const auto inTaggEff = cmd_taggEff->getValue();
    const auto inBkg2    = cmd_bkg2->getValue();

    LOG(INFO) << "Processed files for Setup " << processFiles(inBkg1,inTaggEff,inBkg2) << ".";

    return EXIT_SUCCESS;
}
