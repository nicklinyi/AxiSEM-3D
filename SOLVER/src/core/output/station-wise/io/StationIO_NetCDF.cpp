//
//  StationIO_NetCDF.cpp
//  AxiSEM3D
//
//  Created by Kuangdai Leng on 3/24/19.
//  Copyright © 2019 Kuangdai Leng. All rights reserved.
//

//  serial NetCDF IO for station output

#include "StationIO_NetCDF.hpp"
#include "io.hpp"
#include "mpi.hpp"
#include "vector_tools.hpp"

// initialize
void StationIO_NetCDF::initialize(const std::string &groupName,
                                  int numRecordSteps,
                                  const std::vector<std::string> &channels,
                                  const std::vector<std::string> &stKeys) {
    // finalize
    finalize();
    
    // base
    StationIO::initialize(groupName, numRecordSteps, channels, stKeys);
    
    // need rank_station.info
    createRankStation(groupName, stKeys);
    
    // nothing locally without stations
    if (stKeys.size() == 0) {
        return;
    }
    
    // filename
    const std::string &gdir = io::gOutputDirectory + "/stations/" + groupName;
    const std::string &fname =
    gdir + "/axisem3d_synthetics.nc.rank" + mpi::strRank();
    
    // create and open
    mNcFile = std::make_unique<NetCDF_Writer>();
    mNcFile->open(fname, true);
    
    // reset time line
    mFileLineTime = 0;
    
    ///////////////////// define variables /////////////////////
    mNcFile->defModeOn();
    
    // time
    mVarID_Time = mNcFile->defineVariable("time_points", {
        {"dim_time", numRecordSteps}}, numerical::dErr);
    
    // data
    mVarID_Data = mNcFile->defineVariable("data_wave", {
        {"dim_station", stKeys.size()},
        {"dim_channel", channels.size()},
        {"dim_time", numRecordSteps}
    }, (numerical::Real)numerical::dErr);
    
    // channels
    mNcFile->defineVariable("channel_order", {
        {"dim_channel", channels.size()},
        {"dim_channel_str_length", vector_tools::maxLength(channels)}
    }, (char)0);
    
    // stations
    mNcFile->defineVariable("station_order", {
        {"dim_station", stKeys.size()},
        {"dim_station_str_length", vector_tools::maxLength(stKeys)}
    }, (char)0);
    
    // end defining variables
    mNcFile->defModeOff();
    
    ///////////////////// write info /////////////////////
    // write channels
    for (int ich = 0; ich < channels.size(); ich++) {
        mNcFile->writeVariable("channel_order", channels[ich],
                               {ich, 0}, {1, (int)channels[ich].size()});
    }
    
    // write station keys
    for (int ist = 0; ist < stKeys.size(); ist++) {
        mNcFile->writeVariable("station_order", stKeys[ist],
                               {ist, 0}, {1, (int)stKeys[ist].size()});
    }
}

// finalize
void StationIO_NetCDF::finalize() {
    // close files
    if (mNcFile) {
        mNcFile->close();
        mNcFile.reset(nullptr);
    }
    mFileLineTime = 0;
}

// dump to file
void StationIO_NetCDF::dumpToFile(const eigen::DColX &bufferTime,
                                  const eigen::RTensor3 &bufferFields,
                                  int bufferLine) {
    // no station
    int nst = (int)bufferFields.dimensions()[0];
    if (nst == 0) {
        return;
    }
    
    // no line
    if (bufferLine == 0) {
        return;
    }
    
    // write time
    mNcFile->writeVariable(mVarID_Time, "time_points", bufferTime,
                           {mFileLineTime}, {bufferLine});
    
    // write data
    int nch = (int)bufferFields.dimensions()[1];
    mNcFile->writeVariable(mVarID_Data, "data_wave", bufferFields,
                           {0, 0, mFileLineTime},
                           {nst, nch, bufferLine});
    
    // flush?
    // mNcFile->flush();
    
    // update record postion in file
    mFileLineTime += bufferLine;
}
