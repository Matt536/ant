#include "OutputManager.h"

#include <iostream>
#include <string>
#include "base/Logger.h"
#include "base/std_ext.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include "TFile.h"
#include "TDirectory.h"
#pragma GCC diagnostic pop

using namespace std;
using namespace ant;
using namespace ant::output;



OutputManager::OutputManager() : files()
{
    current_dir = gDirectory;
}

void OutputManager::SetNewOutput(const string &filename)
{

    try {
        auto f = std_ext::make_unique<TFileWrapper>(filename);
        current_dir = **f;
        files.emplace_back( std::move(f) );
        VLOG(5) << "Current root output directory is " << current_dir->GetPath();
    } catch (...) {
        cerr << "Can't open output file " << filename << endl;
        current_dir = gDirectory;
    }

}

OutputManager::TFileWrapper::TFileWrapper(const string &filename)
{
    TFile* f = new TFile(filename.c_str(), "RECREATE");
    if(f && f->IsOpen()) {
        VLOG(5) << "Opened output file " << filename;
        file = f;
    } else
        throw false;
}

OutputManager::TFileWrapper::~TFileWrapper()
{
    if(file) {
        if(file->IsOpen()) {
            VLOG(5) << "Syncing output file " << file->GetName();
            file->Write();
            file->Close();
            VLOG(5) << "Closed output file " << file->GetName();
        }
        delete file;
    }
}
